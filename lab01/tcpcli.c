#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inte.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define SRV_PORT 49152
#define IP_ADDR "131.113.110.80"
#define BUFSIZE 100

int main(int argc, char const* argv[])
{
	// INITIALIZATION
	int s, msglen, cnt;
	socklen_t srvlen, fromlen;
	struct sockaddr_in srvskt, from;
	char *input[BUFSIZE];
	fprintf(stderr, "CONNECTING %s:%d\n", IP_ADDR, PORT_NUM);
	// CLIENT
	return 0;
}
