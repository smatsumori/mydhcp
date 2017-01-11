#include "./mydhcpc.h"
#define EV_INIT 1
#define EV_EPSILON 2
#define EV_SEND_DISCOVER 3
#define EV_TIMEOUT 13
#define EV_RECVOFFER_C0 14
#define EV_RECVOFFER_C1 15
#define EV_RECVACK_C0 16
#define EV_RECVACK_C4 17
#define EV_TIMER_TICK_HALF 18
#define EV_SIGHUP 19

#define ST_INIT 1
#define ST_SEND_DISCOVER 2
#define ST_WAIT_OFFER 3
#define ST_WAIT_ACK 4
#define ST_IN_USE 5
#define ST_EXIT 6


int wait_event(struct dhcphead *);
static int status;
static int alrmflag, sighupflag = 0;

static struct eventtable etab[] = {
	{EV_INIT, "EV_INIT", ""},
	{EV_SEND_DISCOVER, "EV_SEND_DISCOVER","DHCPDISCOVER has sent."},
	{EV_TIMEOUT, "EV_TIMEOUT", "Timeout."},
	{EV_RECVOFFER_C0, "EV_RECVOFFER_C0", "Offer Code=0 recieved."},
	{EV_RECVOFFER_C1, "EV_RECVOFFER_C1", "No IP address available this time."},
	{EV_RECVACK_C0, "EV_RECVACK_C0", "IP addres has allocated."},
	{EV_RECVACK_C4, "EV_RECVACK_C4", "Error with REQUEST."},
	{EV_TIMER_TICK_HALF, "EV_TIMER_TICK_HALF", "Passed half of ttl(time to live)"},
	{EV_SIGHUP, "EV_SIGHUP", "Signal recieved."},
	{EV_EPSILON, "EV_EPSILON", ""},
	{0, "", ""}
};

static struct eventtable stab[] = {
	{ST_INIT, "ST_INIT", ""},
	{ST_SEND_DISCOVER, "ST_SEND_DISCOVER", ""},
	{ST_WAIT_OFFER, "ST_WAIT_OFFER", ""},
	{ST_WAIT_ACK, "ST_WAIT_ACK", ""},
	{ST_IN_USE, "ST_IN_USE", ""},
	{ST_EXIT, "ST_EXIT", ""},
	{0, "", ""}
};

static struct proctable ptab[]= {
	{ST_INIT, EV_INIT, init, ST_SEND_DISCOVER},
	{ST_INIT, EV_SIGHUP, send_release, ST_EXIT},
	{ST_SEND_DISCOVER, EV_SEND_DISCOVER, send_discover, ST_WAIT_OFFER},
	{ST_SEND_DISCOVER, EV_SIGHUP, send_release, ST_EXIT},
	{ST_WAIT_OFFER, EV_TIMEOUT, send_discover, ST_WAIT_OFFER},		/* DISCOVER TIMEOUT */
	{ST_WAIT_OFFER, EV_RECVOFFER_C0, send_request, ST_WAIT_ACK},
	{ST_WAIT_OFFER, EV_RECVOFFER_C1, resend_discover, ST_WAIT_OFFER},
	{ST_WAIT_OFFER, EV_SIGHUP, send_release, ST_EXIT},
	{ST_WAIT_ACK, EV_RECVACK_C0, start_use, ST_IN_USE},
	{ST_WAIT_ACK, EV_RECVACK_C4, resend_request, ST_WAIT_ACK},
	{ST_WAIT_ACK, EV_SIGHUP, send_release, ST_EXIT},
	{ST_IN_USE, EV_TIMER_TICK_HALF, send_extend, ST_WAIT_ACK},
	{ST_IN_USE, EV_SIGHUP, send_release, ST_EXIT},
	{ST_EXIT, EV_EPSILON, exit_client, ST_EXIT},
	{0, 0, NULL, 0}	/* Sentinel */
};

static struct dhcphead dhcph = {
	.mysocd = -1
};


void sighup_func()
{
	fprintf(stderr, "SIGHUP recieved\n");
	sighupflag++;
	return;
}

void alrm_func()
{	
	/* called when it recieves SIGALRM */
	alrmflag++;
	return;
}

int main(int argc, char const* argv[])
{
	char ipaddr[16] = DHCP_SERV_IPADDR;
	#ifdef DEBUG
		fprintf(stderr, "Running on DEBUG mode\n");
	#endif
	#ifndef DEBUG
		if (argc != 2) {
			fprintf(stderr, "Usage: ./mydhcpc.out <server IP>\n");
			exit(1);
		} else {
			strcpy(ipaddr, argv[1]);
		}
	#endif
	struct proctable *ptptr;
	struct dhcphead *hpr = &dhcph;


	signal(SIGHUP, sighup_func);		// handle SIGHUP
	int event = EV_INIT;	/* Initialization */
	status = ST_INIT;
	fprintf(stderr, "\n--------STATUS: %2d--------\n", status);
	while (1) {
		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == status && ptptr -> event == event) {
					(*ptptr -> func)(hpr);
					status = ptptr->nextstatus;
					fprintf(stderr, "moving to status: %2d\n\n", status);
					print_status(status, stab);
					break;
				}
		}
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Hit sentinel. Processing error in main()");
	
		if ((event = wait_event(hpr)) < 0)
			report_error_and_exit(ERROR_EVENT, "main");
		print_event(event, etab);
	}

	return 0;
}

int wait_event(struct dhcphead *hpr)
{
	static struct itimerval itval;
	struct timeval dtime;
	switch (status) {
		case ST_INIT:
			/* error if it passes here */
			break;
		case ST_SEND_DISCOVER:
			return EV_SEND_DISCOVER;		// DISCOVER has sent

		case ST_WAIT_OFFER:
			assert(hpr->mysocd != -1);
			switch (recvoffer(hpr)) {
				case -1:		// timeout
					return EV_TIMEOUT;
				case 0:		// recvoffer(code 0)
					fprintf(stderr, "Time to live: %d\n", hpr->servttl);
					return EV_RECVOFFER_C0;
				case 1:		// recv offer(code 1)	failed
					return EV_RECVOFFER_C1;
			}

		case ST_WAIT_ACK:
			switch (recvack(hpr)) {
				case -1:		// timeout
					return EV_TIMEOUT;
				case 0:
					return EV_RECVACK_C0;
				case 1:
					return EV_RECVACK_C4;
			}

		case ST_IN_USE:
			signal(SIGALRM, alrm_func);		// handle SIGHUP
			gettimeofday(&dtime, NULL);		// get current time TODO: remove this
			dtime.tv_sec += ((long)hpr->servttl);
			itval.it_value.tv_sec = ((long)hpr->servttl) / 2;
			itval.it_value.tv_usec = 0;
			itval.it_interval.tv_sec, itval.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &itval, NULL);
			fprintf(stderr, "Timer stops in: %ld secs\n", itval.it_value.tv_sec);
			pause();
			if (sighupflag > 0){
				return EV_SIGHUP;
			} else if (alrmflag > 0) {		// if SIGALRM was recived
				alrmflag = 0;
				return EV_TIMER_TICK_HALF;
			}
			break;

		case ST_EXIT:
			return EV_EPSILON;
	}
	return -1;
}
