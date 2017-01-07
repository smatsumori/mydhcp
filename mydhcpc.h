#ifndef __MYDHCPC__
#define __MYDHCPC__
/* Header File */
#include "./packet.h"
#include <assert.h>
#include <sys/select.h>

/* Defines */
#define MAX_NAME 20
#define MAX_DESCRIPTION 100
#define DHCP_SERV_IPADDR "192.168.1.101"
#define ERR_PROCESSING 10
#define ERR_SENDTO 11
#define ERR_SELECT 12
#define ERR_RECVFROM 13

/* dhcphead used for passing socket descriptor for the machine
 * and socket for dhcp server
 */
struct dhcphead{
	int mysocd; // socket descriptor
	struct sockaddr_in *socaddptr;		/* socket for server */
};


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

/* Q01 */
void init(struct dhcphead *hpr)
{
	// Get IP from stdin
	assert(hpr->mysocd == -1);	/* Haven't initialized yet */
	fprintf(stderr, "Initialization\n");
	
	/* Begin initialization */
	int socd;
	struct sockaddr_in skt;		// soc for serv
	struct in_addr ipaddr;	// ipaddr for dhcp serv
	socd = socket(PF_INET, SOCK_DGRAM, 0);		// get socket descriptor

	/* set server socket */
	assert(inet_aton(DHCP_SERV_IPADDR, &ipaddr) == 1);	// set dhcp serv ip
	fprintf(stderr, "DHCP server's IP has set to: %s\n", DHCP_SERV_IPADDR);
	skt.sin_family = AF_INET;		// set address family
	skt.sin_port = htons(DHCP_SERV_PORT);		// port num
	skt.sin_addr.s_addr = htonl(ipaddr.s_addr);		// set ipaddr

	hpr->socaddptr = &skt;
	hpr->mysocd = socd;

	fprintf(stderr, "Initialization cmpl\n");
	return;
}

/* Q02 */
void send_discover(struct dhcphead *hpr)
{
	fprintf(stderr, "Send Discover\n");

	struct dhcp_packet dpacket = {
		.op = DHCP_DISCOVER, .siaddr = hpr->socaddptr->sin_addr, 
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT
	};
	
	int dsize;		/* rval is data size sent */
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: DISCOVER\n");
	fprintf(stderr, "SIZE: %d\n", dsize);
	
	return;
}

void close(struct dhcphead *hpr)
{
	
}

/*** event functions ***/
int recvoffer(struct dhcphead *hpr)
{
	/* returns
	 * -1: timeout
	 *  0: recv offer
	 *  1: recv offer (error)
	 */
	/* ports already set */
	int rv, count;
	socklen_t sktlen;		// size of servers socket length
	struct dhcp_packet recvpacket;
	struct timeval timeout = {		// set timeout for DHCPDISCOVER to 5 secs
	.tv_sec = 5,
	};
	// no need to bind socket
	fd_set rdfds;		/* sets of file descriptor */
	FD_ZERO(&rdfds);		/* set fds to 0 */
	FD_SET(hpr->mysocd, &rdfds);		/* set file descriptor */
	if ((rv = select(hpr->mysocd, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvoffer");
	} else if (rv == 0) {
		fprintf(stderr, "Time out. No data after 5 secs.\n");
		return -1;
	} else {		// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);
			if ((count = recvfrom(hpr->mysocd, &recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen))) {
				report_error_and_exit(ERR_RECVFROM, "recvoffer");
			}
		}
	}
	return -2;
}


#endif	/* __MYDHCPC__ */
