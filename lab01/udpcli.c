#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inte.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define PORT_NUM 49152
#define IP_ADDR "131.113.110.80"
#define BUFSIZE 100

void report_error_and_exit(char const *, int);

int main(int argc, char const* argv[])
{
	// INITIALIZATION
	int s, msglen, cnt;
	char msg[80], *p;
	char *input[BUFSIZE];
	struct sockaddr_in srvskt, from;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
		report_error_and_exit("socket", 1);

	srvskt.sin_family = AF_INET;
	srvskt.sin_port = htons(PORT_NUM);

	if (inet_aton(IP_ADDR, &srvskt.sin_addr))
		report_error_and_exit("inte_aton", 1);

	fprintf(stderr, "CONNECTING %s:%d\n", IP_ADDR, PORT_NUM);

	// CLIENT
	return 0;
}

void report_error_and_exit(char const *msg, int errno) {
	perror(msg);
	exit(errno);
}
