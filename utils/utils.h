#ifndef __MYDHCPUTILS__
#define __MYDHCPUTILS__


/*** DEFINES ***/
#define MAX_NAME 25
#define MAX_DESCRIPTION 100
#define MSG_TIMEOUT 10
#define DHCP_SERV_IPADDR "131.113.108.53"

/*** ERROR DEFINES ***/
#define ERR_PROCESSING 10
#define ERR_SENDTO 11
#define ERR_SELECT 12
#define ERR_RECVFROM 13
#define ERROR_EVENT 14
#define ERR_ATON 111
#define ERR_INVALID_OP 112
#define ERR_SOCKET 113
#define ERR_BIND 114
#define ERR_INVALID_CODE 115
#define ERR_READ_CONFIG 213
#define ERR_MALLOC 214


/*** INCLUDES ***/

#include <assert.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <strings.h>
#include <string.h>

/*** MACROS ***/

/*** FSM ***/
struct eventtable {
	int id;
	char name[MAX_NAME];
	char description[MAX_DESCRIPTION];
};

/*** UTILS ***/
void print_event(int id, struct eventtable *etabp)
{
	struct eventtable *evptr;
	for (evptr = etabp; evptr->id != 0; evptr++) {
		if (evptr->id == id) {
			fprintf(stderr, "## Event:%2d :: %2s :: %s ##\n",
				 	evptr->id, evptr->name, evptr->description);
			return;
		}
	}
	fprintf(stderr, "error: print_event\n");
	return;
}

void print_status(int id, struct eventtable *stabp)
{
	struct eventtable *stptr;
	for (stptr = stabp; stptr->id != 0; stptr++) {
		if (stptr->id == id) {
			fprintf(stderr, "--------- status %2d: %s ---------\n", stptr->id, stptr->name);
			return;
		}
	}
	fprintf(stderr, "error: print_status\n");
	return;
}

#endif
