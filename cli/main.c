/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/defaults.h>
#include <groundlift/client.h>
#include <groundlift/server.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private methods. */
void sigint_handler(int sig);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	int ret;

	/* Setup our defaults. */
	ret = 0;

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Start as server or as client. */
	if ((argc < 2) || (argv[1][0] == 's')) {
		/* Initialize the server. */
		if (!gl_server_init(NULL, TCPSERVER_PORT)) {
			fprintf(stderr, "Failed to initialize the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Start it up. */
		if (!gl_server_start()) {
			fprintf(stderr, "Failed to start the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Wait for it to return. */
		if (!gl_server_thread_join()) {
			fprintf(stderr, "Server thread returned with errors.\n");

			ret = 1;
			goto cleanup;
		}
	} else if ((argc == 4) && (argv[1][0] == 'c')) {
		/* Initialize the client. */
		if (!gl_client_init(argv[2], (uint16_t)atoi(argv[3]))) {
			fprintf(stderr, "Failed to initialize the client.\n");

			ret = 1;
			goto cleanup;
		}

		/* Connect to the server. */
		if (!gl_client_connect()) {
			fprintf(stderr, "Failed to connect the client to the server.\n");

			ret = 1;
			goto cleanup;
		}

		/* Wait for it to return. */
		if (!gl_client_thread_join()) {
			fprintf(stderr, "Client thread returned with errors.\n");

			ret = 1;
			goto cleanup;
		}
	} else {
		printf("Unknown mode or invalid number of arguments.\n");
		return 1;
	}

cleanup:
	/* Free up any resources. */
	gl_server_free();
	gl_client_free();

	return ret;
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
	printf("Got a Ctrl-C\n");

	/* Disconnect the client. */
	gl_client_disconnect();

	/* Stop the server. */
	gl_server_stop();

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}
