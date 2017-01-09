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

/*** DHCPHEAD ***/
struct dhcphead{
	int ipsets;
	int client_id;
	int mysocd;
	struct client *clisthpr;		// client list head pointer
	struct ippool *iplisthpr;		// server list head pointer
};

/*** FUNCTIONS ***/
void global_init(struct dhcphead *hpr)
{
	fprintf(stderr, "Initializing Server...\n");

	if ((hpr->mysocd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		report_error_and_exit(ERR_SOCKET, "global_init");

	struct sockaddr_in myskt;
	bzero(&myskt, sizeof myskt);
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(DHCP_SERV_PORT);
	assert(inet_aton(DHCP_SERV_IPADDR, &myskt.sin_addr) == 1);
	fprintf(stderr, "DHCP server's IP: %s:%d\n", 
			inet_ntoa(myskt.sin_addr), DHCP_SERV_PORT);

	if (bind(hpr->mysocd, (struct sockaddr *)&myskt, sizeof myskt) < 0)
		report_error_and_exit(ERR_BIND, "global_init");

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

/*** FSM ACTION ***/
void init(struct dhcphead *hpr)
{
	// TODO: implement
	return;
}


#endif
