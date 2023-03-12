/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "mdnscommon.h"
#include "tcpserver.h"
#ifdef USE_AVAHI
#include "avahi.h"
#endif

/* Private methods. */
server_err_t test_server(void);
mdns_err_t test_mdns(void);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	return test_server();
}

/**
 * Tests out the server functionality.
 *
 * @return Server return code.
 */
server_err_t test_server(void) {
	return server_start(NULL, 1234);
}

/**
 * Tests out the mDNS functionality.
 *
 * @return mDNS return code.
 */
mdns_err_t test_mdns(void) {
	mdns_err_t err;

	/* Initialize mDNS client and start the event loop. */
	err = mdns_init();
	if (err)
		return err;
	mdns_event_loop();

	/* Free up everything related to mDNS. */
	mdns_free();

	return MDNS_OK;
}
