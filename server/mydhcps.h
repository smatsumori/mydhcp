#ifndef __MYDHCPS__
#define __MYDHCPS__
/*** Header File ***/
#include "../utils/packet.h"
#include "../utils/utils.h"
#include "./client.h"
#include <string.h>
#include <assert.h>

/*** ERROR ***/
// TODO: move to utils
#define ERR_SOCKET 113
#define ERR_BIND 114

/*** PROTOTYPES ***/
struct dhcphead;
struct ippool;
struct eventtable;
struct proctable;

void set_iptab(struct dhcphead *, struct ippool *, struct ippool *);
struct client *set_cltab(struct dhcphead *hpr, struct client *clhpr, struct client *cpr);

/*** DHCPHEAD ***/
struct dhcphead{
	int ipsets;
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

	fprintf(stderr, "Initialization complete!\n");

	return;
}


/*** FSM ***/

struct eventtable {
	int id;
	char name[MAX_NAME];
	char description[MAX_DESCRIPTION];
};


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
struct client *set_cltab(struct dhcphead *hpr, struct client *clhpr, struct client *cpr)
{
	// TODO: remove clhpr (don't need this)
	/* malloc for cpr */
	struct client *cpr_m;
	if ((cpr_m = (struct client *)malloc(sizeof(struct client))) == NULL) 
		report_error_and_exit(ERR_MALLOC, "set_cltab");

	*cpr_m = *cpr;		// copy instance 

	cpr_m->id = ++(hpr->clients_online);

	cpr_m->fp = clhpr;
	cpr_m->bp = clhpr->bp;
	clhpr->bp->fp = cpr_m;
	clhpr->bp = cpr_m;

	printf("[NEW CLIENT] ID:%2d  ", cpr_m->id);
	printf("IPADDR(ID): %s  ", inet_ntoa(cpr_m->addr));
	return cpr_m;
}

struct client *find_cltab(struct dhcphead *hpr, struct in_addr id_addr)
{
	/* find designated client with in_addr */
	return NULL;
}


void set_iptab(struct dhcphead *hpr, struct ippool *iphpr, struct ippool *ipr)
{
	/* malloc for ipr */
	struct ippool *ipr_m;
	if ((ipr_m = (struct ippool *)malloc(sizeof(struct ippool))) == NULL)
		report_error_and_exit(ERR_MALLOC, "set_iptab");

	*ipr_m = *ipr;		// copy (not sure if this works)
	ipr_m->id = ++(hpr->ipsets);	/* set number of ips */

	printf("[SET] ID:%2d  ", ipr_m->id);
	printf("IPADDR: %s  ", inet_ntoa(ipr_m->addr));
	printf("NETMASK: %s\n", inet_ntoa(ipr_m->netmask));

	/* set ip to ippool */
	ipr_m->fp = iphpr;
	ipr_m->bp = iphpr->bp;
	iphpr->bp->fp = ipr_m;
	iphpr->bp = ipr_m;

	return;
}

void get_iptab(struct ippool *iphpr)
{
	return;
}

void print_all_iptab(struct ippool *iphpr)
{
	fprintf(stderr, "\n*** LIST OF IPTABLE***\n");
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
	return;
}

/*** GLOBAL ***/
int recvpacket(struct dhcphead *hpr)
{
	/* set dhcp packet */

	int rv, count;
	socklen_t sktlen; // size of client's socket
	struct timeval timeout = {
		.tv_sec = MSG_TIMEOUT,
	};

	// socket already binded
	
	fd_set rdfds;		// sets of socket descriptor
	FD_ZERO(&rdfds);
	FD_SET(hpr->mysocd, &rdfds);		// set file descriptor

	if ((rv = select(hpr->mysocd + 1, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvmsg");
	} else if (rv == 0) {		// timeout
		fprintf(stderr, "Time out. No data after %d secs.\n", MSG_TIMEOUT);
		return -1;
	} else {	// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);
			if ((count = recvfrom(hpr->mysocd, &hpr->recvpacket, sizeof hpr->recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen)) < 0) {		// recv message
				report_error_and_exit(ERR_RECVFROM, "recvmsg");
			}
			printf("DATA RECIEVED\n");
			// TODO: FIX FAITAL BUG
			printf("FROM: %s LENGTH: %d \n", inet_ntoa(hpr->socaddptr->sin_addr), count);
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


#endif
