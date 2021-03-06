#ifndef __CLIENT_HEADER__
#define __CLIENT_HEADER__

/* client status */
#define CLI_SEND_DISCOVER 10
#define CLI_WAIT_OFFER 11
#define CLI_WAIT_ACK 12
#define CLI_IN_USE 13

struct ippool;

struct client {
	struct client *fp;	/* linked list */
	struct client *bp;
	int id;
	short status;
	int ttlcounter;		/* remaining time */

	/*** below: network byte order ***/
	struct in_addr id_addr;		/* client IP */
	struct ippool *ippptr;	/* pointer for IP pool */
	struct in_addr addr;		/* client IP (allocated) */
	struct in_addr netmask;		/* netmask (allocated) */
	in_port_t port;		/* port */
	uint16_t ttl;		/* time to live */
};

#endif
