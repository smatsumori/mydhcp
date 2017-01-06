#ifndef __CLIENT_HEADER__
#define __CLIENT_HEADER__
struct client {
	struct client *fp;	/* linked list */
	struct client *bp;
	short status;
	int ttlcounter;		/* remaining time */
	// below: network byte order
	struct in_addr id;
	struct in_addr addr;
	struct in_addr netmask;
	uint16_t ttl;
};

struct client client_list;	// list header
#endif
