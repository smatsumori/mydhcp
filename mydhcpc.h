#ifndef __MYDHCPC__
#define __MYDHCPC__
/* Header File */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* Defines */
#define DHCP_IPADDR "192.168.1.101"
#define DHCP_SERV_PORT 68
#define ERR_PROCESSING 10

struct dhcphead{
	int socd;	// socket descriptor
	struct sockaddr_in *socptr;
};

struct proctable {
	int status;
	int event;	/* input */
	void (*func)(struct dhcphead *hpr);
};

/* Q00 */
void init(struct dhcphead *hpr)
{
	// Get IP from stdin
	assert(hpr->socd = -1);	/* Haven't initialized yet */
	fprintf(stderr, "Initialization\n");
	
	/* Begin initialization */
	int socd;
	struct sockaddr_in skt;		// soc for serv
	struct in_addr ipaddr;	// ipaddr for dhcp serv
	socd = socket(PF_INET, SOCK_DGRAM, 0);		// get socket descriptor

	/* server */
	assert(inet_aton(DHCP_IPADDR, &ipaddr) == 1);	// set dhcp serv ip
	fprintf(stderr, "DHCP server's IP has deliberately set: %s\n", DHCP_IPADDR);
	skt.sin_family = AF_INET;		// set address family
	skt.sin_port = htons(DHCP_SERV_PORT);		// port num
	skt.sin_addr.s_addr = htonl(ipaddr.s_addr);		// set ipaddr

	hpr->socptr = &skt;
	hpr->socd = socd;

	fprintf(stderr, "Initialization cmpl\n");
	return;
}



void report_error_and_exit(int errno, const char *msg)
{
	fprintf(stderr, "Runtime Error: %s\n", msg);
	exit(errno);
}
#endif	/* __MYDHCPC__ */
