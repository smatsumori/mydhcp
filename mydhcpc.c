#include "./mydhcpc.h"
#define EV_INIT 1
#define EV_SEND_DISCOVER 2
#define EV_TIMEOUT 13
#define EV_RECVOFFER_C0 14
#define EV_RECVOFFER_C1 15

#define ST_INIT 1
#define ST_SEND_DISCOVER 2
#define ST_WAIT_OFFER 3
#define ST_WAIT_ACK 4
#define ST_IN_USE 5
#define ST_EXIT 6


int wait_event(struct dhcphead *);
static int status;

static struct eventtable etab[] = {
	{EV_INIT, "EV_INIT", ""},
	{EV_SEND_DISCOVER, "EV_SEND_DISCOVER","DHCPDISCOVER has sent."},
	{EV_TIMEOUT, "EV_TIMEOUT", "Timeout."},
	{EV_RECVOFFER_C0, "EV_RECVOFFER_C0", "Offer recieved."},
	{EV_RECVOFFER_C1, "EV_RECVOFFER_C1", "No IP address available this time."},
};

static struct proctable ptab[]= {
	{ST_INIT, EV_INIT, init, ST_SEND_DISCOVER},
	{ST_SEND_DISCOVER, EV_SEND_DISCOVER, send_discover, ST_WAIT_OFFER},
	{ST_WAIT_OFFER, EV_TIMEOUT, send_discover, ST_WAIT_OFFER},		/* DISCOVER TIMEOUT */
	{ST_WAIT_OFFER, EV_RECVOFFER_C0, send_request, ST_WAIT_ACK},
	{ST_WAIT_OFFER, EV_RECVOFFER_C1, send_request, ST_WAIT_OFFER},
	{0, 0, NULL}	/* Sentinel */
};

static struct dhcphead dhcph = {
	.mysocd = -1
};

void print_event(int id)
{
	struct eventtable *evptr;
	for (evptr = etab; evptr->id != 0; evptr++) {
		if (evptr->id == id) {
			fprintf(stderr, "Event:%2d :: %2s :: %s\n",
				 	evptr->id, evptr->name, evptr->description);
			return;
		}
	}
	fprintf(stderr, "error: print_event\n");
	return;
}

int main(int argc, char const* argv[])
{
	struct proctable *ptptr;
	struct dhcphead *hpr = &dhcph;
	int event = EV_INIT;	/* Initialization */
	status = ST_INIT;
	fprintf(stderr, "\n--------STATUS: %2d--------\n\n", status);
	while (1) {
		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == status && ptptr -> event == event) {
					(*ptptr -> func)(hpr);
					status = ptptr->nextstatus;
					fprintf(stderr, "moving to status: %2d\n", status);
					fprintf(stderr, "\n--------STATUS: %2d--------\n\n", status);
					break;
				}
		}
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Hit sentinel. Processing error in main()");
	
		event = wait_event(hpr);
		print_event(event);
	}

	return 0;
}

int wait_event(struct dhcphead *hpr)
{
	switch (status) {
		case ST_SEND_DISCOVER:
			return EV_SEND_DISCOVER;		// DISCOVER has sent

		case ST_WAIT_OFFER:
			assert(hpr->mysocd != -1);
			switch (recvoffer(hpr)) {
				case -1:		// timeout
					return EV_TIMEOUT;
				case 0:		// recvoffer(code 0)
					return EV_RECVOFFER_C0;
				case 1:		// recv offer(code 1)	failed
					return EV_RECVOFFER_C1;
			}
			break;
	}
	return 0;
}
