#ifndef __MYDHCPS__
#define __MYDHCPS__
/*** Header File ***/
#include "../utils/packet.h"
#include "../utils/utils.h"
#include "./client.h"

/*** ERROR ***/
// TODO: move to utils
#define VALID_PACKET 1
#define INVALID_PACKET -1
#define CONNECTION_TIMEOUT 10

/*** PROTOTYPES ***/
struct dhcphead;
struct ippool;
struct eventtable;
struct proctable;

void set_iptab(struct dhcphead *, struct ippool *, struct ippool *);
struct client *set_cltab(struct dhcphead *hpr, struct client *clhpr, struct client *cpr);
struct client *find_cltab(struct dhcphead *hpr, struct in_addr id_addr, in_port_t id_port);
void update_cltab_ttlcnt(struct dhcphead *hpr, int consumed);
struct client *get_tout_client(struct dhcphead *hpr);
void remove_cltab(struct dhcphead *hpr);
void print_all_cltab(struct client *clhpr);
struct ippool *get_iptab(struct ippool *iphpr);
void print_all_iptab(struct ippool *iphpr);



/*** DHCPHEAD ***/
struct dhcphead{
	int ipsets;
	int clients;
	int clients_online;
	int client_id;
	int mysocd;
	struct dhcp_packet recvpacket;
	struct sockaddr_in *socaddptr;
	struct client *cliincmd;		// client now in command
	struct client *clisthpr;		// client list head pointer
	struct ippool *iplisthpr;		// server list head pointer
};

/*** FUNCTIONS ***/
void global_init(struct dhcphead *hpr)
{
	fprintf(stderr, "Initializing Server...\n");

	/** CLIENT **/
	if((hpr->socaddptr = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in))) == NULL)
		report_error_and_exit(ERR_MALLOC, "global_init");
	bzero(hpr->socaddptr, sizeof *(hpr->socaddptr));		// necessary

	/** SERVER **/
	if ((hpr->mysocd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		report_error_and_exit(ERR_SOCKET, "global_init");

	struct sockaddr_in myskt;
	bzero(&myskt, sizeof myskt);
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(DHCP_SERV_PORT);
	assert(inet_aton(DHCP_SERV_IPADDR, &myskt.sin_addr) == 1);

	if (bind(hpr->mysocd, (struct sockaddr *)&myskt, sizeof myskt) < 0)
		report_error_and_exit(ERR_BIND, "global_init");

	fprintf(stderr, "bind cmpl...\n");

	fprintf(stderr, "DHCP server's IP: %s:%d\n", 
			inet_ntoa(myskt.sin_addr), DHCP_SERV_PORT);

	fprintf(stderr, "Initialization complete!\n\n\n");

	return;
}


/*** FSM ***/

struct proctable {
	int status;
	int event;	/* input */
	void (*func)(struct dhcphead *hpr);
	int nextstatus;
};


/*** LINKED LIST ***/
struct ippool {
	int id;
	struct ippool *fp;		/* linked list */
	struct ippool *bp;
	/*** below: network byte order ***/
	struct in_addr addr;
	struct in_addr netmask;
	uint16_t ttl;
};


/*** ACCESSOR ***/
/*** CLIENT TABLE ***/
struct client *set_cltab(struct dhcphead *hpr, struct client *clhpr, struct client *cpr)
{
	// TODO: remove clhpr (don't need this)
	/* malloc for cpr */
	//fprintf(stderr, "\nInserting new client\n");
	struct client *cpr_m;
	if ((cpr_m = (struct client *)malloc(sizeof(struct client))) == NULL) 
		report_error_and_exit(ERR_MALLOC, "set_cltab");

	*cpr_m = *cpr;		// copy instance 
	cpr_m->id = ++(hpr->clients);

	cpr_m->fp = clhpr;
	cpr_m->bp = clhpr->bp;
	clhpr->bp->fp = cpr_m;
	clhpr->bp = cpr_m;
	printf("[NEW CLIENT] ID:%2d  ", cpr_m->id);
	printf("IPADDR(ID): %s\n", inet_ntoa(cpr_m->id_addr));

	/* set IP for a rent */
	cpr_m->ippptr = get_iptab(hpr->iplisthpr);

	/* set ttlcount */
	return cpr_m;
}

struct client *find_cltab(struct dhcphead *hpr, struct in_addr id_addr, in_port_t id_port)
{
	/* find designated client with in_addr */
	struct client *sptr;

	for (sptr = hpr->clisthpr->fp; sptr->id != 0; sptr = sptr->fp) {
		if (sptr->id_addr.s_addr == id_addr.s_addr && sptr->port == id_port)		// found
			return sptr;
	}
	return NULL;
}

void update_cltab_ttlcnt(struct dhcphead *hpr, int consumed)
{
	
	struct client *sptr;
	for (sptr = hpr->clisthpr->fp; sptr->id != 0; sptr = sptr->fp) {
		sptr->ttlcounter -= consumed;
		if (0 < sptr->ttlcounter) {
			fprintf(stderr, "CLIENT %2d: remaining %2d\n", sptr->id, sptr->ttlcounter);
		}
	}
	return;
}

struct client *get_tout_client(struct dhcphead *hpr)
{
	struct client *sptr;
	for (sptr = hpr->clisthpr->fp; sptr->id != 0; sptr = sptr->fp) {
		if (sptr->ttlcounter <= 0) {
			fprintf(stderr, "CLIENT %2d: Timeout!(%2d secs)\n", sptr->id, 10 -(sptr->ttlcounter));
			return sptr;
		}
	}
	return NULL;
}

void remove_cltab(struct dhcphead *hpr)
{
	struct client *sptr = hpr->cliincmd;
	fprintf(stderr, "Removing client ID: %2d\n", sptr->id);
	sptr->bp->fp = sptr->fp;
	sptr->fp->bp = sptr->bp;
	free(sptr);
	return;
}

void print_all_cltab(struct client *clhpr)
{
	fprintf(stderr, "\n*** LIST OF CLIENTS ***\n");
	struct client *sp = clhpr;
	for (sp = sp->fp; sp->id != 0; sp = sp->fp) {
		printf("[ID:%2d]  ", sp->id);
		printf("IPADDR(ID): %s ", inet_ntoa(sp->id_addr));
		printf("IPADDR: %s  ", inet_ntoa(sp->addr));
		printf("NETMASK: %s  ", inet_ntoa(sp->netmask));
		printf("TTLCOUNTER: %2d\n", sp->ttlcounter);
	}
	return;
}

/**** IPTABLES ****/
void set_iptab(struct dhcphead *hpr, struct ippool *iphpr, struct ippool *ipr)
{
	/* malloc for ipr */
	struct ippool *ipr_m;
	if ((ipr_m = (struct ippool *)malloc(sizeof(struct ippool))) == NULL)
		report_error_and_exit(ERR_MALLOC, "set_iptab");

	*ipr_m = *ipr;		// copy (not sure if this works)
	ipr_m->id = ++(hpr->ipsets);	/* set number of ips */

	ipr_m->ttl = hpr->iplisthpr->ttl;

	/* set ip to ippool */
	ipr_m->fp = iphpr;
	ipr_m->bp = iphpr->bp;
	iphpr->bp->fp = ipr_m;
	iphpr->bp = ipr_m;

	printf("[SET] ID:%2d  ", ipr_m->id);
	printf("IPADDR: %s  ", inet_ntoa(ipr_m->addr));
	printf("NETMASK: %s\n", inet_ntoa(ipr_m->netmask));

	return;
}

struct ippool *get_iptab(struct ippool *iphpr)
{
	/* get ip from ip pool */
	struct ippool *sp = iphpr;
	
	sp = iphpr->fp;
	sp->fp->bp = iphpr;
	sp->bp->fp = sp->fp;

	/* remove connections */
	sp->fp = sp;
	sp->bp = sp;

	fprintf(stderr, "IP for rent: %s\n", inet_ntoa(sp->addr));

	/*
	fprintf(stderr, "\n***  REMAINING  ***\n");
	print_all_iptab(iphpr);
	*/
	return sp;
}

void print_all_iptab(struct ippool *iphpr)
{
	fprintf(stderr, "\n*** LIST OF IPTABLE ***\n");
	struct ippool *sp = iphpr;
	for (sp = sp->fp; sp->id != 0; sp = sp->fp) {
		printf("[SET] ID:%2d  ", sp->id);
		printf("IPADDR: %s  ", inet_ntoa(sp->addr));
		printf("NETMASK: %s\n", inet_ntoa(sp->netmask));
	}
	return;
}

void free_iptab(struct ippool *iphpr)
{
	struct ippool *sp = iphpr;
	struct ippool *np;
	for (sp = sp->fp; sp != iphpr;) {
		np = sp->fp;
		free(sp);
		sp = np;
	}
	return;
}

/*** GLOBAL ***/
int recvpacket(struct dhcphead *hpr)
{
	/* set dhcp packet */

	int rv, count;
	socklen_t sktlen; // size of client's socket
	struct timeval timeout = {
		.tv_sec = 0,
	};

	// socket already binded
	
	fd_set rdfds;		// sets of socket descriptor
	FD_ZERO(&rdfds);
	FD_SET(hpr->mysocd, &rdfds);		// set file descriptor

	if ((rv = select(hpr->mysocd + 1, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvmsg");
	} else if (rv == 0) {		// timeout
		fprintf(stderr, "[NO PACKET] Waiting...\r", MSG_TIMEOUT);
		return -1;
	} else {	// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);		// socaddptr = client sockaddr
			if ((count = recvfrom(hpr->mysocd, &hpr->recvpacket, sizeof hpr->recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen)) < 0) {		// recv message
				report_error_and_exit(ERR_RECVFROM, "recvmsg");
			}
			printf("[SUCCSESS] DATA RECIEVED ");
			printf("FROM: %s", inet_ntoa(hpr->socaddptr->sin_addr));
			printf(":%d LENGTH: %d \n", ntohs(hpr->socaddptr->sin_port), count);
			return 0;
		}
	}

}

/*** FSM ACTION ***/
void init(struct dhcphead *hpr)
{
	// TODO: implement
	return;
}

void send_offer(struct dhcphead *hpr)
{
	// TODO: impl CODE_ERR
	fprintf(stderr, "Send OFFER\n");
	struct dhcp_packet dpacket = {		// do we really need ciaddr?
		.op = DHCP_OFFER, .code = CODE_OK, 
		.ttl = hpr->iplisthpr->ttl, .ciaddr = hpr->cliincmd->id_addr, 
		.ciport = hpr->cliincmd->port, .ipaddr = hpr->cliincmd->ippptr->addr,
		.netmask = hpr->cliincmd->ippptr->netmask
	};

	/* set client socket from recvpacket information */
	/* DO WE NEED THIS? */
	hpr->socaddptr->sin_family = AF_INET;
	//hpr->socaddptr->sin_port = htons(DHCP_CLI_PORT);		// TODO: add port to struct client
	hpr->socaddptr->sin_addr = hpr->cliincmd->id_addr;
	hpr->socaddptr->sin_port = hpr->cliincmd->port;
	
	fprintf(stderr, "Seinding OFFER to: %s", inet_ntoa(hpr->socaddptr->sin_addr));
	fprintf(stderr, ":%d\n", ntohs(hpr->socaddptr->sin_port));
	int dsize;		/* rval is data size sent */
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "[SUCCSESS]SENT: OFFER ");
	fprintf(stderr, "SIZE: %d ", dsize);
	fprintf(stderr, "For rent: %s\n", inet_ntoa(dpacket.ipaddr));

	/* set timeout */
	hpr->cliincmd->ttlcounter = CONNECTION_TIMEOUT;
	fprintf(stderr, "Wait offer for %d\n", hpr->cliincmd->ttlcounter);
	return;
}

void send_ack(struct dhcphead *hpr)
{
	// TODO: implement CODE_ERR
	fprintf(stderr, "Send ACK\n");
	struct dhcp_packet dpacket = {		// do we really need ciaddr?
		.op = DHCP_ACK, .code = CODE_OK, 
		.ttl = hpr->iplisthpr->ttl, .ciaddr = hpr->cliincmd->id_addr, 
		.ciport = hpr->cliincmd->port, .ipaddr = hpr->cliincmd->ippptr->addr,
		.netmask = hpr->cliincmd->ippptr->netmask
	};

	/* set client socket from recvpacket information */
	/* DO WE NEED THIS? */
	hpr->socaddptr->sin_family = AF_INET;
	//hpr->socaddptr->sin_port = htons(DHCP_CLI_PORT);		// TODO: add port to struct client
	hpr->socaddptr->sin_addr = hpr->cliincmd->id_addr;
	hpr->socaddptr->sin_port = hpr->cliincmd->port;
	
	fprintf(stderr, "Seinding ACK to: %s", inet_ntoa(hpr->socaddptr->sin_addr));
	fprintf(stderr, ":%d\n", ntohs(hpr->socaddptr->sin_port));
	int dsize;		/* rval is data size sent */
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	// TODO: check user's ttl
	fprintf(stderr, "[SUCCSESS]SENT: ACK ");
	fprintf(stderr, "SIZE: %d ", dsize);
	fprintf(stderr, "Allow using: %s\n", inet_ntoa(dpacket.ipaddr));

	/* set timeout */
	hpr->cliincmd->ttlcounter = (int)hpr->cliincmd->ttl;
	return;
}

void client_exit(struct dhcphead *hpr)
{
	// TODO: implement
	
	set_iptab(hpr, hpr->iplisthpr, hpr->cliincmd->ippptr);
	remove_cltab(hpr);
	fprintf(stderr, "Client Exit\n");
	return;
}


/*** EVENT FUCNTION ***/
int dhcp_packet_handler(struct dhcphead *hpr, int *code, int *errno)
{
	switch (hpr->recvpacket.op) {
		case DHCP_DISCOVER:
			fprintf(stderr, "Message is DHCP_DISCOVER\n");
			return DHCP_DISCOVER;
		
		case DHCP_REQUEST:
			fprintf(stderr, "Message is DHCP_REQUEST\n");
			if (hpr->recvpacket.ipaddr.s_addr != hpr->cliincmd->ippptr->addr.s_addr) {
				fprintf(stderr, "INVALID IP\n");
				*errno = INVALID_PACKET;
				return DHCP_REQUEST;
			} else if (hpr->recvpacket.netmask.s_addr != hpr->cliincmd->ippptr->netmask.s_addr) {
				fprintf(stderr, "INVALID NETMASK\n");
				*errno = INVALID_PACKET;
				return DHCP_REQUEST;
			} else if (hpr->recvpacket.ttl > hpr->cliincmd->ippptr->ttl) {
				fprintf(stderr, "INVALID TTL\n");
				*errno = INVALID_PACKET;
				return DHCP_REQUEST;
			} else {		// IF valid
				hpr->cliincmd->ttl = hpr->recvpacket.ttl;
				*errno = VALID_PACKET;
				*code = hpr->recvpacket.code;
				return DHCP_REQUEST;
			}

	}
	return 0;
}

#endif
