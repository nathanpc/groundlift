/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/conf.h>
#include <groundlift/defaults.h>
#include <groundlift/obex.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "server.h"

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
	gl_err_t *err;
	int ret;

	/* Setup our defaults. */
	ret = 0;
	err = NULL;

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Initialize some common modules. */
	obex_init();
	conf_init();

	/* Start as server or as client. */
	if ((argc < 2) || (argv[1][0] == 's')) {
		err = server_run(NULL, TCPSERVER_PORT);
	} else if ((argc == 5) && (argv[1][0] == 'c')) {
		/* Exchange some information with the server. */
		err = client_send(argv[2], (uint16_t)atoi(argv[3]), argv[4]);
	} else if ((argc < 2) || (argv[1][0] == 'l')) {
		/* List peers on the network. */
		err = client_list_peers();
	} else {
		printf("Unknown mode or invalid number of arguments.\n");
		return 1;
	}

	/* Check if we had any errors to report. */
	gl_error_print(err);
	if (err != NULL)
		ret = 1;

	/* Free up any resources. */
	gl_server_free(g_server);
	gl_client_free(g_client);
	gl_error_free(err);
	conf_free();

	return ret;
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
	printf("Got a Ctrl-C\n");

	/* Abort any ongoing operation. */
	gl_client_disconnect(g_client);
	gl_client_discovery_abort(g_discovery_client);
	gl_server_stop(g_server);

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}
