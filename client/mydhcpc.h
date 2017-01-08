#ifndef __MYDHCPC__
#define __MYDHCPC__
/* Header File */
#include "../utils/packet.h"
#include <assert.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

/* Defines */
#define MAX_NAME 20
#define MAX_DESCRIPTION 100
#define MSG_TIMEOUT 10
#define DHCP_SERV_IPADDR "131.113.108.53"
#define ERR_PROCESSING 10
#define ERR_SENDTO 11
#define ERR_SELECT 12
#define ERR_RECVFROM 13
#define ERROR_EVENT 14

#define ERR_INVALID_OP 112
#define ERR_INVALID_CODE 113
/* dhcphead used for passing socket descriptor for the machine
 * and socket for dhcp server
 */
struct dhcphead{
	int mysocd; // socket descriptor
	struct sockaddr_in *socaddptr;		/* socket for server */

	/* packet info */
	u_int16_t servttl;		/* upper time to live */
	struct in_addr ipaddr;
	struct in_addr netmask;
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
	static struct sockaddr_in skt;		// socket for server SHOULD BE STATIC
	struct in_addr ipaddr;	// ipaddr for dhcp serv
	socd = socket(PF_INET, SOCK_DGRAM, 0);		// get socket descriptor

	fprintf(stderr, "Socket descriptor: %d\n", socd);
	fprintf(stderr, "DHCP server's IP has set to: %s\n", DHCP_SERV_IPADDR);
	/* set server socket */
	assert(inet_aton(DHCP_SERV_IPADDR, &ipaddr) == 1);	// set dhcp serv ip
	ipaddr.s_addr = htonl(ipaddr.s_addr);		// converting to network byte order
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
	fprintf(stderr, "Send DISCOVER\n");

	struct dhcp_packet dpacket = {
		.op = DHCP_DISCOVER, .siaddr = hpr->socaddptr->sin_addr, 
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT
	};
	
	fprintf(stderr, "Seinding DISCOVER to: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
	int dsize;		/* rval is data size sent */
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: DISCOVER\n");
	fprintf(stderr, "SIZE: %d\n", dsize);
	
	return;
}

void resend_discover(struct dhcphead *hpr)
{
	fprintf(stderr, "Resend DISCOVER in 5secs.\n");
	sleep(5);
	send_discover(hpr);
	return;
}


void send_request(struct dhcphead *hpr)
{
	fprintf(stderr, "Send REQUEST\n");
	struct dhcp_packet dpacket = {
		.op = DHCP_REQUEST, .code = CODE_REQALLOC, .siaddr = hpr->socaddptr->sin_addr,
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT,
		.ttl = hpr->servttl, .ipaddr = hpr->ipaddr, .netmask = hpr->netmask
	};

	fprintf(stderr, "Seinding REQUEST to: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
	fprintf(stderr, "Requesting IP: %s\n", inet_ntoa(hpr->ipaddr));

	int dsize;
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: REQUEST\n");
	fprintf(stderr, "SIZE: %d\n", dsize);

	return;
}

void resend_request(struct dhcphead *hpr)
{
	fprintf(stderr, "Resend REQUEST in 5secs.\n");
	sleep(5);
	send_request(hpr);
	return;
}

void send_extend(struct dhcphead *hpr)
{
	fprintf(stderr, "Send REQUEST(Extend)\n");
	struct dhcp_packet dpacket = {
		.op = DHCP_REQUEST, .code = CODE_REQEXTEND, .siaddr = hpr->socaddptr->sin_addr,
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT,
		.ttl = hpr->servttl, .ipaddr = hpr->ipaddr, .netmask = hpr->netmask
	};

	fprintf(stderr, "Seinding REQUEST(Extend) to: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
	fprintf(stderr, "Requesting IP: %s\n", inet_ntoa(hpr->ipaddr));

	int dsize;
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: REQUEST(Extend)\n");
	fprintf(stderr, "SIZE: %d\n", dsize);

	return;
}

void show_ip(struct dhcphead *hpr)
{
	fprintf(stderr, "Address in use\n");
	fprintf(stderr, "IPADDR: %s\n", inet_ntoa(hpr->ipaddr));
	fprintf(stderr, "Time to live: %d\n", hpr->servttl);
	return;
}

void send_release(struct dhcphead *hpr)
{
	fprintf(stderr, "Send RELEASE");
	struct dhcp_packet dpacket = {
		.op = DHCP_RELEASE, .code = 0, .siaddr = hpr->socaddptr->sin_addr,
		.siport = hpr->socaddptr->sin_port, .ciport = DHCP_CLI_PORT,
		.ttl = 0, .ipaddr = hpr->ipaddr, .netmask = 0 
	};

	fprintf(stderr, "Seinding RELEASE to: %s\n", inet_ntoa(hpr->socaddptr->sin_addr));
	fprintf(stderr, "Releasing IP: %s\n", inet_ntoa(hpr->ipaddr));

	int dsize;
	if ((dsize = sendto(hpr->mysocd, &dpacket, sizeof dpacket, 0,
					(struct sockaddr *)(hpr->socaddptr), sizeof *(hpr->socaddptr))) < 0) {
		report_error_and_exit(ERR_SENDTO, "sendto");
	}
	fprintf(stderr, "SENT: RELEASE\n");
	fprintf(stderr, "SIZE: %d\n", dsize);

	return;
}

void exit_client(struct dhcphead *hpr)
{
	fprintf(stderr, "***** Exiting Client *****\n");
	exit(0);
	return;
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
	socklen_t sktlen;		// size of server's socket length
	struct dhcp_packet recvpacket;
	struct timeval timeout = {		// set timeout for DHCPDISCOVER to 5 secs
	.tv_sec = MSG_TIMEOUT,
	};
	// no need to bind socket (because we have no IP)
	fd_set rdfds;		/* sets of file descriptor */
	FD_ZERO(&rdfds);		/* set fds to 0 */
	FD_SET(hpr->mysocd, &rdfds);		/* set file descriptor */

	if ((rv = select(hpr->mysocd + 1, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvoffer");
	} else if (rv == 0) {
		fprintf(stderr, "Time out. No data after %d secs.\n", MSG_TIMEOUT);
		return -1;
	} else {		// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);
			if ((count = recvfrom(hpr->mysocd, &recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen))) {
				report_error_and_exit(ERR_RECVFROM, "recvoffer");
				
				if (recvpacket.op == DHCP_OFFER) {
					switch (recvpacket.code) {
						case CODE_OK:
							hpr->servttl = recvpacket.ttl;
							hpr->ipaddr = recvpacket.ipaddr;
							hpr->netmask = recvpacket.netmask;
							return 0;
						case CODE_ERR:
							return 1;
						default:
							report_error_and_exit(ERR_INVALID_CODE, "recvoffer");
					}
				} else {
					report_error_and_exit(ERR_INVALID_OP, "recvoffer");
				}

			}
		}
	}
	return -2;
}

int recvack(struct dhcphead *hpr)
{
	/* returns
	 * -1: timeout
	 *  0: recv ack
	 *  1: recv ack (error)
	 */
	int rv, count;
	socklen_t sktlen;		// size of server's socket length
	struct dhcp_packet recvpacket;
	struct timeval timeout = {		// set timeout for DHCPACK to 5 sec
		.tv_sec = MSG_TIMEOUT,
	};

	// no need to bind socket (because we have no IP)
	fd_set rdfds;		/* sets of file descriptor */
	FD_ZERO(&rdfds);		/* set fds to 0 */
	FD_SET(hpr->mysocd, &rdfds);		/* set file descriptor */

	if ((rv = select(hpr->mysocd + 1, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvoffer");
	} else if (rv == 0) {
		fprintf(stderr, "Time out. No data after %d secs.\n", MSG_TIMEOUT);
		return -1;
	} else {		// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);
			if ((count = recvfrom(hpr->mysocd, &recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen))) {
				report_error_and_exit(ERR_RECVFROM, "recvoffer");
				
				if (recvpacket.op == DHCP_OFFER) {
					switch (recvpacket.code) {
						case CODE_OK:
							hpr->servttl = recvpacket.ttl;
							hpr->ipaddr = recvpacket.ipaddr;
							hpr->netmask = recvpacket.netmask;
							return 0;
						case CODE_ACKERR:
							return 1;
						default:
							report_error_and_exit(ERR_INVALID_CODE, "recvack");
					}
				} else {
					report_error_and_exit(ERR_INVALID_OP, "recvack");
				}

			}
		}
	}

	return -2;
}

#endif	/* __MYDHCPC__ */
