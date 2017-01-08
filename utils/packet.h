#ifndef __MYDHCPPACKET__
#define __MYDHCPPACKET__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define DHCP_SERV_PORT 51267
#define DHCP_CLI_PORT 51268

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DCHP_NACK 6
#define DHCP_RELEASE 7

#define CODE_OK 0
#define CODE_ERR 1
#define CODE_REQALLOC 2
#define CODE_REQEXTEND 3
#define CODE_ACKERR 4

struct dhcp_packet {
	/** IP HEADER **/
	struct in_addr ciaddr;	/* client IP address */
	struct in_addr siaddr;	/* server IP address */

	/** UDP HEADER **/
	in_port_t ciport;		/* client port */
	in_port_t siport;		/* serv port */

	/** DHCP MSG **/
	u_int8_t op;		/* packet type */
	u_int8_t code;
	u_int16_t ttl;		/* time to live */

	struct in_addr ipaddr;
	struct in_addr netmask;

};


void report_error_and_exit(int errno, const char *msg)
{
	fprintf(stderr, "Runtime error\n");
	perror(msg);
	exit(errno);
}


#endif	/* __MYDHCPPACKET__ */
