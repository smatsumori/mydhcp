#include "./mydhcpc.h"
#define EV_INIT 1
#define EV_SEND_DISCOVER 2
#define EV_TIMEOUT 13

#define ST_INIT 1
#define ST_SEND_DISCOVER 2


int wait_event(struct dhcphead *);
static int status;

static struct proctable ptab[]= {
	{ST_INIT, EV_INIT, init},
	{ST_SEND_DISCOVER, EV_SEND_DISCOVER, send_discover},
	{ST_SEND_DISCOVER, EV_TIMEOUT, send_discover},		/* DISCOVER TIMEOUT */
	{0, 0, NULL}	/* Sentinel */
};

static struct dhcphead dhcph = {
	.socd = -1
};

int main(int argc, char const* argv[])
{
	struct proctable *ptptr;
	struct dhcphead *hpr = &dhcph;
	int event = EV_INIT;	/* Initialization */
	status = ST_INIT;

	while (1) {
		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == status && ptptr -> event == event) {
					(*ptptr -> func)(hpr);
					break;
				}
		}
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Processing error in main()");
		event = wait_event(hpr);
	}

	return 0;
}

int wait_event(struct dhcphead *hpr)
{
	switch (status) {
		case ST_SEND_DISCOVER:
			assert(hpr->socd != -1);
			/* recieve a packet from server */
			/* handle timeout */
			// todo: call recvoffer
			break;
	}
	return 0;
}
