#include "./mydhcps.h"
#define MAX_CONFIG 1024


/*** FSM ***/
#define GLOBAL_EV_RECVMSG 100
#define GLOBAL_EV_TIMEOUT 101
#define GLOBAL_EV_CLI_TIMEOUT 102

#define EV_TIMEOUT 13

#define EV_INIT 1
#define EV_RECV_DISCOVER 200
#define EV_RECV_REQUEST_C2 201
#define EV_RECV_REQUEST_C3 202
#define EV_RECV_RELEASE 203
#define EV_RECV_DISCOVER_NOIP 204
#define EV_IMD_RELEASE 205

#define ST_INIT 1
#define ST_WAIT_DISCOVER 111
#define ST_WAIT_REQUEST 112
#define ST_WAIT_REQUEST_RE 12
#define ST_IP_RENTED 113
#define ST_EXIT 6
/*** PROTOTYPES ***/
int wait_event(struct dhcphead *);
int global_event_dispatcher(struct dhcphead *);
int global_client_selector(struct dhcphead *);

static int global_status;
static int global_event = EV_INIT;

/*** ALARM ***/
int global_alrm_counter = 0;
void GBAlrm(int sig)
{
	global_alrm_counter++;
	return;
}


/*** STRUCT ***/
static struct client clist_head = {
	.id = 0,
	.fp = &clist_head, .bp = &clist_head
};

static struct ippool iplist_head = {
	.id = 0, .fp = &iplist_head, .bp = &iplist_head
};

static struct dhcphead dhcph = {
	.client_id = 0, .clisthpr = &clist_head, .iplisthpr = &iplist_head,
	.cliincmd = &clist_head, 
	.ipsets = 0, .clients_online = 0, .clients = 0
};

static struct eventtable etab[] = {
	{EV_INIT, "EV_INIT", ""},
	{EV_RECV_DISCOVER, "EV_RECV_DISCOVER", "Recieve DISCOVER"},
	{EV_RECV_DISCOVER_NOIP, "EV_RECV_DISCOVER_NOIP", "Recieve DISCPVER but no IP available."},
	{EV_RECV_REQUEST_C2, "EV_RECV_REQUEST_C2", "Recieve REQUEST Code = 2 (ALLOC)"},
	{EV_RECV_REQUEST_C3, "EV_RECV_REQUEST_C3", "Recieve REQUEST Code = 3 (EXTEND)"},
	{EV_RECV_RELEASE, "EV_RECV_RELEASE", "Recieve RELEASE"},
	{EV_IMD_RELEASE, "EV_IMD_RELEASE", "Invalid event."},
	{EV_TIMEOUT, "EV_TIMEOUT", "Client timeout"},
	{0, "", ""}
};

static struct eventtable stab[] = {
	{ST_WAIT_DISCOVER, "ST_WAIT_DISCOVER", ""},
	{ST_WAIT_REQUEST, "ST_WAIT_REQUEST", ""},
	{ST_WAIT_REQUEST_RE, "ST_WAIT_REQUEST_RE", ""},
	{ST_IP_RENTED, "ST_IP_RENTED", ""},
	{ST_EXIT, "ST_EXIT", ""},
	{0, "", ""}
};

static struct proctable ptab[]= {		
	{ST_WAIT_DISCOVER, EV_RECV_DISCOVER, send_offer, ST_WAIT_REQUEST},
	{ST_WAIT_DISCOVER, EV_RECV_DISCOVER_NOIP, client_exit, ST_EXIT},
	{ST_WAIT_DISCOVER, EV_TIMEOUT, client_exit, ST_EXIT},
	{ST_WAIT_DISCOVER, EV_IMD_RELEASE, client_exit, ST_EXIT},
	{ST_WAIT_REQUEST, EV_RECV_REQUEST_C2, send_ack, ST_IP_RENTED},
	{ST_WAIT_REQUEST, EV_TIMEOUT, send_offer, ST_WAIT_REQUEST_RE},
	{ST_WAIT_REQUEST, EV_IMD_RELEASE, client_exit, ST_EXIT},
	{ST_WAIT_REQUEST_RE, EV_TIMEOUT, client_exit, ST_EXIT},		/* timeout 2 times */
	{ST_IP_RENTED, EV_RECV_REQUEST_C3, send_ack, ST_IP_RENTED},
	{ST_IP_RENTED, EV_RECV_RELEASE, client_exit, ST_EXIT},
	{ST_IP_RENTED, EV_TIMEOUT, client_exit, ST_EXIT},
	{ST_IP_RENTED, EV_IMD_RELEASE, client_exit, ST_EXIT},
	{ST_EXIT, 0, NULL, 0},
	{0, 0, NULL, 0}	/* Sentinel */
};

/*** FUNCTIONS ***/

void get_config(const char *configfile, struct dhcphead *hpr)
{
	fprintf(stderr, "Reading config...\n");
	static struct ippool iplist_tmp;
	const char *delim = "\t";
	char *ptr, addr[16], netmask[16];		// TODO: remove magic no
	char cfgstr[MAX_CONFIG];
	FILE *fp;
	int ip_id = 1;
	if ((fp = fopen(configfile, "r")) == NULL) 
		report_error_and_exit(ERR_READ_CONFIG, "get_config");

	if (fgets(cfgstr, MAX_CONFIG, fp) == NULL) 
		report_error_and_exit(ERR_READ_CONFIG, "get_config");

	if ((iplist_head.ttl = atoi(cfgstr)) == 0)		// read time to live
		report_error_and_exit(ERR_READ_CONFIG, "get_config");

	fprintf(stderr, "Time to live (Global) set to:%4d\n", hpr->iplisthpr->ttl);
	fprintf(stderr, "Setting IPs\n");

	while (fgets(cfgstr, MAX_CONFIG, fp) != NULL) {
		/* set ip to iptable */
		if ((ptr = strtok(cfgstr, delim)) == NULL)		// ptr = first token(ipaddr)
			report_error_and_exit(ERR_READ_CONFIG, "get_config");

		strcpy(addr, ptr);

		if (inet_aton(addr, &iplist_tmp.addr) == 0)		// set ipaddr
			report_error_and_exit(ERR_READ_CONFIG, "get_config");

//		iplist_tmp.addr.s_addr = htonl(iplist_tmp.addr.s_addr);		// convert to network order

		if ((ptr = strtok(NULL, delim)) == NULL)		// ptr = second token(netmask)
			report_error_and_exit(ERR_READ_CONFIG, "get_config");

		strcpy(netmask, ptr);

		if (inet_aton(netmask, &iplist_tmp.netmask) == 0)		// set netmask
			report_error_and_exit(ERR_READ_CONFIG, "get_config");

//		iplist_tmp.netmask.s_addr = htonl(iplist_tmp.netmask.s_addr);		// convert to network order

		set_iptab(hpr, hpr->iplisthpr, &iplist_tmp);
	}
	
	return;
}


/*** MAIN ***/
int main(int argc, char const* argv[])
{
	struct proctable *ptptr;
	struct dhcphead *hpr = &dhcph;		// must access through hpr

	#ifndef DEBUG
	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcps.out config-file\n");
		exit(1);
	} else {
		get_config(argv[1], hpr);
	}
	#endif

	#ifdef DEBUG
	get_config("config-file", hpr);
	#endif

	/* Alarm handling */

	global_init(hpr);

	int event = EV_INIT;
	global_status = ST_INIT;

	while(1) {
		global_event = global_event_dispatcher(hpr);		// get global event
		if (global_event == GLOBAL_EV_TIMEOUT)
			continue;
		if (global_event != GLOBAL_EV_CLI_TIMEOUT){
			global_client_selector(hpr);		// select client in command
		}


		/** below executing client FSM **/
		fprintf(stderr, "\x1b[32m");		// color green
		fprintf(stderr, "\n\n********Now taking care of CLIENT: %2d********\n", hpr->cliincmd->id);
		print_status(hpr->cliincmd->status, stab);

		event = wait_event(hpr);
		print_event(event, etab);

		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == hpr->cliincmd->status && ptptr -> event == event) {
					(*ptptr -> func)(hpr);
					hpr->cliincmd->status = ptptr->nextstatus;
					fprintf(stderr, "moving to status: %2d\n", hpr->cliincmd->status);
					fprintf(stderr, "------------- STATUS END --------------\n\n\n", 
							hpr->cliincmd->status);
					break;
				}
		}
		fprintf(stderr, "\x1b[39m");		// color default
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Hit sentinel. Processing error in main()");

	}

	return 0;
}

int global_event_dispatcher(struct dhcphead *hpr) 
{
	/* distinguishes who to throw an event */
	signal(SIGALRM, GBAlrm);
	static struct itimerval itval;
	itval.it_value.tv_sec = 1;
	itval.it_value.tv_usec, itval.it_interval.tv_sec, itval.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &itval, NULL);
	pause();
	if (global_alrm_counter > 1) {
		update_cltab_ttlcnt(hpr, global_alrm_counter);
		global_alrm_counter = 0;
		if ((hpr->cliincmd = get_tout_client(hpr)) != NULL) {
			fprintf(stderr, "GLOBAL_EV_CLI_TIMEOUT\n");
			return GLOBAL_EV_CLI_TIMEOUT;
		}
	}

	switch (recvpacket(hpr)) {
		case -1:
			return GLOBAL_EV_TIMEOUT;

		case 0:		// message recieved
			return GLOBAL_EV_RECVMSG;

	}
	return 0;		// error
}

int wait_event(struct dhcphead *hpr)
{
	
	if (global_event == GLOBAL_EV_CLI_TIMEOUT) 
		return EV_TIMEOUT;
	
	int code, errno;
	switch (hpr->cliincmd->status) {
		case ST_WAIT_DISCOVER:
			switch (dhcp_packet_handler(hpr, &code, &errno)) {
				case DHCP_DISCOVER:
					if ((hpr->ipsets - hpr->clients_online) == 0) 
						return EV_RECV_DISCOVER_NOIP;		// no ip available
					return EV_RECV_DISCOVER;
			}
		
		case ST_WAIT_REQUEST_RE:
		case ST_WAIT_REQUEST:
			switch (dhcp_packet_handler(hpr, &code, &errno)) {
				case DHCP_REQUEST:
					if (errno != INVALID_PACKET) {
						switch (code) {
							case CODE_REQALLOC:	// proper
								return EV_RECV_REQUEST_C2;
							case CODE_REQEXTEND:
								return EV_RECV_REQUEST_C3;
						}
					} else {		// if INVALID
						return EV_IMD_RELEASE;	//TODO: return EV
					}
			}

		case ST_IP_RENTED:
			// TODO: check timeout
			switch(dhcp_packet_handler(hpr, &code, &errno)) {
				case DHCP_REQUEST:
					if (errno != INVALID_PACKET) {
						switch (code) {
							case CODE_REQALLOC:
								return EV_RECV_REQUEST_C2;
							case CODE_REQEXTEND:		// proper
								return EV_RECV_REQUEST_C3;
						}
					} else {		// if INVALID
						return EV_IMD_RELEASE;	//TODO: return EV
					}
			}
			break;

		case ST_EXIT:
			// TODO: impelement
			break;

		default:
			break;
	}
	return EV_IMD_RELEASE;
}

int global_client_selector(struct dhcphead *hpr){
	/* select client by IP */
	/* if client doesn't exist then create a new one */
	struct dhcp_packet packet = hpr->recvpacket;
	struct client *cli;
	static struct client newclient;

	if ((cli = find_cltab(hpr, hpr->socaddptr->sin_addr, 
					hpr->socaddptr->sin_port)) == NULL) {	// not found
		//fprintf(stderr, "Client NOT found: IPADDR: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
		newclient.id_addr = hpr->socaddptr->sin_addr;		// set ID(ipaddr) for a new client
		newclient.port = hpr->socaddptr->sin_port;
		newclient.status = ST_WAIT_DISCOVER;
		newclient.ttlcounter = 20;		// TODO: remove magicno
		hpr->cliincmd = set_cltab(hpr, hpr->clisthpr, &newclient);		// set new client to table
		print_all_cltab(hpr->clisthpr);
	} else {		// found
		//fprintf(stderr, "Client found: ID:%2d IPADDR: %s\n", cli->id, inet_ntoa(cli->id_addr));
		hpr->cliincmd = cli;
	}
	return 0;
}
