/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/conf.h>
#include <groundlift/defaults.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/capabilities.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif /* _WIN32 */

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
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
#endif /* _WIN32 */

	/* Setup our defaults. */
	ret = 0;
	err = NULL;

#ifdef _WIN32
	/* Initialize the Winsock stuff. */
	wVersionRequested = MAKEWORD(2, 2);
	if ((ret = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		printf("WSAStartup failed with error %d\n", ret);
		return 1;
	}
#endif /* _WIN32 */

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Initialize some common modules. */
	cap_init();
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
		ret = 1;
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

#ifdef _WIN32
	/* Clean up the Winsock stuff. */
	WSACleanup();
#endif /* _WIN32 */

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
