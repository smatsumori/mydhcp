#ifndef __MYDHCPC__
#define __MYDHCPC__
/* Header File */
#include "./packet.h"
#include <assert.h>

/* Defines */
#define DHCP_SERV_IPADDR "192.168.1.101"
#define ERR_PROCESSING 10
#define ERR_SENDTO 11


struct dhcphead{
	int socd;	// socket descriptor
	struct sockaddr_in *socaddptr;		/* socket for server */
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
	assert(inet_aton(DHCP_SERV_IPADDR, &ipaddr) == 1);	// set dhcp serv ip
	fprintf(stderr, "DHCP server's IP has deliberately set: %s\n", DHCP_SERV_IPADDR);
	skt.sin_family = AF_INET;		// set address family
	skt.sin_port = htons(DHCP_SERV_PORT);		// port num
	skt.sin_addr.s_addr = htonl(ipaddr.s_addr);		// set ipaddr

	hpr->socaddptr = &skt;
	hpr->socd = socd;

	fprintf(stderr, "Initialization cmpl\n");
	return;
}

void send_discover(struct dhcphead *hpr)
{
	fprintf(stderr, "Send Discover\n");

	struct dhcp_packet dpacket = {
		.op = DHCP_DISCOVER, .siaddr = hpr->socaddptr->sin_addr, 
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT
	};
	
	int dsize;		/* rval is data size sent */
	if ((dsize = sendto(hpr->socd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: DISCOVER\n");
	fprintf(stderr, "SIZE: %d\n", dsize);
	
	return;
}

#endif	/* __MYDHCPC__ */
