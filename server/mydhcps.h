#ifndef __MYDHCPS__
#define __MYDHCPS__
/*** Header File ***/
#include "../utils/packet.h"
#include "../utils/utils.h"
#include "./client.h"
#include <string.h>

/*** PROTOTYPES ***/
struct dhcphead;
struct ippool;
struct eventtable;
struct proctable;

/*** DHCPHEAD ***/
struct dhcphead{
	int client_id;
	int mysocd;
	struct client *clisthpr;		// client list head pointer
	struct ippool *iplisthpr;		// server list head pointer
};


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
void set_iptab(struct ippool *iphpr, struct ippool *ipr)
{
	/* malloc for ipr */
	struct ippool *ipr_m;
	if ((ipr_m = (struct ippool *)malloc(sizeof(struct ippool))) == NULL)
		report_error_and_exit(ERR_MALLOC, "set_iptab");

	*ipr_m = *ipr;		// copy (not sure if this works)

	/* set ip to ippool */
	ipr_m->fp = iphpr;
	ipr_m->bp = iphpr->bp;
	iphpr->bp->fp = ipr_m;
	iphpr->bp = ipr_m;
	return;
}

void get_iptab(struct ippool *hpr)
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
