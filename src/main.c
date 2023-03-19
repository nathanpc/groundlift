/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "mdnscommon.h"
#include "tcp.h"
#ifdef USE_AVAHI
#include "avahi.h"
#endif

/* Private methods. */
tcp_err_t test_server(void);
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
tcp_err_t test_server(void) {
	tcp_err_t err;
	server_t *server;
	server_conn_t *conn;
	char qc;

	/* Get a server handle. */
	server = tcp_server_new(NULL, 1234);
	if (server == NULL)
		return TCP_ERR_UNKNOWN;

	/* Start the server and listen for incoming connections. */
	err = tcp_server_start(server);
	if (err)
		return err;

	/* Accept incoming connections. */
	qc = '\0';
	while (((conn = tcp_server_conn_accept(server)) != NULL) && (qc != '%')) {
		char buf[100];
		size_t len;

		/* Send some data to the client. */
		err = tcp_server_conn_send(conn, "Hello, world!", 13);
		if (err)
			goto close_conn;

		/* Read some data until a % is sent. */
		while (qc != '%') {
			/* Read incoming data. */
			err = tcp_server_conn_recv(conn, buf, 99, &len, false);
			if (err)
				break;

			/* Properly terminate the received string and print it. */
			buf[len] = '\0';
			printf("%s\n", buf);

			/* Check for the termination character. */
			qc = buf[0];
		}

close_conn:
		/* Close the connection and free up any resources. */
		tcp_server_conn_close(conn);
		tcp_server_conn_free(conn);
	}

	/* Stop the server and free up any resources. */
	err = tcp_server_stop(server);
	tcp_server_free(server);

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
