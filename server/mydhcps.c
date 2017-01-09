#include "./mydhcps.h"
#define MAX_CONFIG 1024


/*** FSM ***/
#define GLOBAL_EV_RECVMSG 100
#define GLOBAL_EV_TIMEOUT 101
#define EV_TIMEOUT 13

#define EV_INIT 1
#define EV_RECV_DISCOVER 200
#define EV_RECV_REQUEST_C2 201
#define EV_RECV_REQUEST_C3 202
#define EV_RECV_RELEASE 203

#define ST_INIT 1
#define ST_SEND_DISCOVER 2
#define ST_WAIT_OFFER 3
#define ST_WAIT_ACK 4
#define ST_IN_USE 5
#define ST_EXIT 6

#define ST_WAIT_DISCOVER 11
#define ST_WAIT_REQUEST 12
#define ST_IP_RENTED 13

/*** PROTOTYPES ***/
int wait_event(struct dhcphead *);
int global_event_dispatcher(struct dhcphead *);
int global_client_selector(struct dhcphead *);

static int global_status;
static struct client clist_head = {
	.fp = &clist_head, .bp = &clist_head
};

static struct ippool iplist_head = {
	.id = 0, .fp = &iplist_head, .bp = &iplist_head
};

static struct dhcphead dhcph = {
	.client_id = 0, .clisthpr = &clist_head, .iplisthpr = &iplist_head,
	.ipsets = 0, .clients_online = 0
};

static struct eventtable etab[] = {
	{EV_INIT, "EV_INIT", ""},
	{EV_RECV_DISCOVER, "EV_RECV_DISCOVER", "Recieve DISCOVER"},
	{EV_RECV_REQUEST_C2, "EV_RECV_REQUEST_C2", "Recieve REQUEST Code = 2 (ALLOC)"},
	{EV_RECV_REQUEST_C3, "EV_RECV_REQUEST_C3", "Recieve REQUEST Code = 3 (EXTEND)"},
	{EV_RECV_RELEASE, "EV_RECV_RELEASE", "Recieve RELEASE"}
};

static struct proctable ptab[]= {		// TODO: handle invalid msg
	{ST_WAIT_DISCOVER, EV_RECV_DISCOVER, send_offer, ST_WAIT_REQUEST},
	{ST_WAIT_REQUEST, EV_RECV_REQUEST_C2, send_ack, ST_IP_RENTED},
	{ST_IP_RENTED, EV_RECV_REQUEST_C3, send_ack, ST_IP_RENTED},
	{ST_IP_RENTED, EV_RECV_RELEASE, client_exit, ST_EXIT},
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

void print_event(int id)
{
	struct eventtable *evptr;
	for (evptr = etab; evptr->id != 0; evptr++) {
		if (evptr->id == id) {
			fprintf(stderr, "##Event:%2d :: %2s :: %s##\n",
				 	evptr->id, evptr->name, evptr->description);
			return;
		}
	}
	fprintf(stderr, "error: print_event\n");
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
		getconfig(argv[1], hpr);
	}
	#endif

	#ifdef DEBUG
	get_config("config-file", hpr);
	#endif

	global_init(hpr);

	int global_event = EV_INIT;
	int event = EV_INIT;
	global_status = ST_INIT;


	while(1) {
		// TODO: show client status
		global_event = global_event_dispatcher(hpr);		// get global event
		global_client_selector(hpr);		// select client in command

		/** below executing client FSM **/
		fprintf(stderr, "\n********Now taking care of CLIENT: %2d********\n\n", hpr->cliincmd->id);
		fprintf(stderr, "\n--------STATUS: %2d--------\n\n", hpr->cliincmd->status);
		event = wait_event(hpr);
		print_event(event);
		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == hpr->cliincmd->status && ptptr -> event == event) {
					(*ptptr -> func)(hpr);
					hpr->cliincmd->status = ptptr->nextstatus;
					fprintf(stderr, "moving to status: %2d\n", hpr->cliincmd->status);
					break;
				}
		}
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Hit sentinel. Processing error in main()");

	}

	return 0;
}

int global_event_dispatcher(struct dhcphead *hpr) 
{
	/* distinguishes who to throw an event */
	// TODO: also need to handle msg timout for each client(how?)
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
	switch (hpr->cliincmd->status) {
		case ST_WAIT_DISCOVER:
			// call get_discover
			return EV_RECV_DISCOVER;		// DISCOVER has sent

	}
	return 0;
}

int global_client_selector(struct dhcphead *hpr)
{
	/* select client by IP */
	/* if client doesn't exist then create a new one */
	struct dhcp_packet packet = hpr->recvpacket;
	struct client *cli;
	struct client newclient;

	fprintf(stderr, "Looking for client...:%s\n", inet_ntoa(hpr->socaddptr->sin_addr));
	
	if ((cli = find_cltab(hpr, hpr->socaddptr->sin_addr)) == NULL) {	// not found
		fprintf(stderr, "Client NOT found: IPADDR: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
		newclient.id_addr = hpr->socaddptr->sin_addr;		// set ID(ipaddr) for a new client
		hpr->cliincmd = set_cltab(hpr, hpr->clisthpr, &newclient);		// set new client to table
	} else {		// found
		fprintf(stderr, "Client found: ID:%2d IPADDR: %s\n", cli->id, inet_ntoa(cli->id_addr));
		hpr->cliincmd = cli;
	}
	return 0;
}
