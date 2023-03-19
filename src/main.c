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
	server_err_t err;
	server_t *server;
	server_conn_t *conn;
	char qc;

	/* Get a server handle. */
	server = server_new(NULL, 1234);
	if (server == NULL)
		return SERVER_ERR_UNKNOWN;

	/* Start the server and listen for incoming connections. */
	err = server_start(server);
	if (err)
		return err;

	/* Accept incoming connections. */
	qc = '\0';
	while (((conn = server_conn_accept(server)) != NULL) && (qc != '%')) {
		char buf[100];
		size_t len;

		if (send(conn->sockfd, "Hello, world!", 13, 0) == -1)
			perror("send");

		while (qc != '%') {
			if ((len = recv(conn->sockfd, buf, 99, 0)) == -1) {
				perror("recv");
				break;
			}
			buf[len] = '\0';

			printf("%s\n", buf);

			qc = buf[0];
		}

		/* Close the connection and free up any resources. */
		server_conn_close(conn);
		server_conn_free(conn);
	}

	/* Stop the server and free up any resources. */
	err = server_stop(server);
	server_free(server);

	return err;
}

#ifdef USE_AVAHI
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
#endif /* USE_AVAHI */
