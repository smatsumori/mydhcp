#include "./mydhcpc.h"
#define EV_INIT 0

#define ST_INIT 0


int wait_event();

static struct proctable ptab[]= {
	{ST_INIT, EV_INIT, init},
	{0, 0, NULL}	/* Sentinel */
};

static struct dhcphead dhcph = {
	-1	
};

int main(int argc, char const* argv[])
{
	struct proctable *ptptr;
	struct dhcphead *dhptr;
	int event = EV_INIT;	/* Initialization */
	int status = ST_INIT;

	while (1) {
		event = wait_event();
		for (ptptr = ptab; ptptr -> status; ptptr++) {
				if (ptptr -> status == status && ptptr -> event == event) {
					(*ptptr -> func)(dhptr);
					break;
				}
		}
		if (ptptr -> status == 0) 
			report_error_and_exit(ERR_PROCESSING, "Processing error in main()");
	}

	return 0;
}

int wait_event()
{

	return 0;
}
