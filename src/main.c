/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef USE_AVAHI
#include "avahi.h"
#endif

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	mdns_err_t err;

	/* Initialize mDNS client and start the event loop. */
	err = mdns_init();
	if (err)
		return err;
	mdns_event_loop();

	/* Free up everything related to mDNS. */
	mdns_free();

	return 0;
}
