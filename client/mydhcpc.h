#ifndef __MYDHCPC__
#define __MYDHCPC__
/* Header File */
#include "../utils/packet.h"
#include "../utils/utils.h"


/* dhcphead used for passing socket descriptor for the machine
 * and socket for dhcp server
 */
struct dhcphead{
	int mysocd;		/* socket descriptor for client */
	struct sockaddr_in *socaddptr;		/* socket for server */

	/* packet info */
	u_int16_t servttl;		/* upper time to live */
	struct in_addr ipaddr;
	struct in_addr netmask;
};

/*** FSM ***/

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
	int suffix;
	printf("Set client port: %d + ", DHCP_CLI_PORT);
	scanf("%d", &suffix);

	if(DHCP_CLI_PORT + suffix == DHCP_SERV_PORT) {
		fprintf(stderr, "Cannot use DHCP_SERV_PORT: %d\n", DHCP_SERV_PORT);
	}

	/* bind client socket */
	int socd;
	static struct sockaddr_in skt;		// socket for server SHOULD BE STATIC
	if ((socd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		report_error_and_exit(ERR_SOCKET, "init");		// get socd
	fprintf(stderr, "Socket descriptor: %d\n", socd);

	struct sockaddr_in myskt;		// socket for client
	bzero(&myskt, sizeof myskt);
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(DHCP_CLI_PORT + suffix);		// NEED TO SET PORT
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);		// any IP for client

	if (bind(socd, (struct sockaddr *)&myskt, sizeof myskt) < 0)
		report_error_and_exit(ERR_BIND, "init");

	fprintf(stderr, "DHCP client's IP: %s", inet_ntoa(myskt.sin_addr));
	fprintf(stderr, ":%d\n", ntohs(myskt.sin_port));


	/* set server socket */
	skt.sin_family = AF_INET;		// set address family
	skt.sin_port = htons(DHCP_SERV_PORT);		// port num
	if (inet_aton(DHCP_SERV_IPADDR, &skt.sin_addr) == 0)
		report_error_and_exit(ERR_ATON, "init");
	fprintf(stderr, "DHCP server's IP has set to: %s\n", DHCP_SERV_IPADDR);

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

void start_use(struct dhcphead *hpr)
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
	close(hpr->mysocd);
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
	fprintf(stderr, "Recieving OFFER\n");

	int rv, count;
	socklen_t sktlen;		// size of server's socket length
	struct dhcp_packet recvpacket;
	struct timeval timeout = {		// set timeout for DHCPDISCOVER to 5 secs
	.tv_sec = MSG_TIMEOUT,
	};
	// no need to bind socket (because we have no IP)
	fd_set rdfds;		/* sets of file descriptor */
	FD_ZERO(&rdfds);		/* set fds to 0 */
	FD_SET(hpr->mysocd, &rdfds);		/* set file client descriptor */


	if ((rv = select(hpr->mysocd + 1, &rdfds, NULL, NULL, &timeout)) < 0) {
		report_error_and_exit(ERR_SELECT, "recvoffer");
	} else if (rv == 0) {
		fprintf(stderr, "[FAIL]Time out. No data after %d secs.\n", MSG_TIMEOUT);
		return -1;
	} else {		// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {
			sktlen = sizeof *(hpr->socaddptr);	// set size

			if ((count = recvfrom(hpr->mysocd, &recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen)) < 0) {
				printf("[FAIL] DATA RECIEVED\n");
				printf("FROM: %s", inet_ntoa(hpr->socaddptr->sin_addr));
				printf(":%d LENGTH: %d \n", hpr->socaddptr->sin_port, count);
				report_error_and_exit(ERR_RECVFROM, "recvoffer:");
			}

			printf("[SUCCSESS] OFFER RECIEVED\n");
			printf("FROM: %s", inet_ntoa(hpr->socaddptr->sin_addr));
			printf(":%d LENGTH: %d \n", hpr->socaddptr->sin_port, count);
			
			if (recvpacket.op == DHCP_OFFER) {
				switch (recvpacket.code) {
					case CODE_OK:
						hpr->servttl = recvpacket.ttl;		// set ttl
						hpr->ipaddr = recvpacket.ipaddr;		// set ipaddress (can allocate)
						hpr->netmask = recvpacket.netmask;		// set netmask
						return 0;
					case CODE_ERR:
						return 1;
					default:
						report_error_and_exit(ERR_INVALID_CODE, "recvoffer: invalidcode");
				}
			} else {
				// TODO: handle invald packet
				fprintf(stderr, "Invalid Packet type: %d\n", recvpacket.op);
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
			sktlen = sizeof *(hpr->socaddptr);		// sockaddptr (server)
			if ((count = recvfrom(hpr->mysocd, &recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *)hpr->socaddptr, &sktlen)) < 0) {
				report_error_and_exit(ERR_RECVFROM, "recvack");
			}

			printf("[SUCCSESS] ACK RECIEVED\n");
			printf("FROM: %s", inet_ntoa(hpr->socaddptr->sin_addr));
			printf(":%d LENGTH: %d \n", hpr->socaddptr->sin_port, count);

			if (recvpacket.op == DHCP_ACK) {
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

	return -2;
}

#endif	/* __MYDHCPC__ */
