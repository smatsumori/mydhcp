#ifndef __MYDHCPS__
#define __MYDHCPS__
/* Header File */
#include "../utils/packet.h"

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
void set_iptab(struct ipool *hpr, struct ippool *ipr)
{
	/* set ip to ippool */
	ipr->fp = hpr;
	ipr->bp = hpr->bp;
	hpr->bp->fp = ipr;
	hpr->bp = ipr;
	return;
}

void get_iptab(struct ippool *hpr)
{
	return;
}

#endif
