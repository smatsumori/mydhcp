#ifndef __MYDHCPPACKET__
#define __MYDHCPPACKET__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define DHCP_SERV_PORT 67
#define DHCP_CLI_PORT 68

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DCHP_NACK 6
#define DHCP_RELEASE 7

struct dhcp_packet {
	u_int8_t op;		/* packet type(MESSAGE) */

	/** IP HEADER **/
	struct in_addr ciaddr;	/* client IP address */
	struct in_addr siaddr;	/* server IP address */

	/** UDP HEADER **/
	in_port_t ciport;		/* client port */
	in_port_t	siport;		/* serv port */
};


void report_error_and_exit(int errno, const char *msg)
{
	fprintf(stderr, "Runtime error\n");
	perror(msg);
	exit(errno);
}


#endif	/* __MYDHCPPACKET__ */
