/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/defaults.h>
#include <groundlift/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

/* Client thread function arguments structure definition. */
typedef struct {
	const char *addr;
	uint16_t port;
} client_thread_args_t;

/* Private variables. */
static tcp_client_t *m_client;
static pthread_t m_client_thread;

/* Private methods. */
void sigint_handler(int sig);
void *client_thread_func(void *arg);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	void *retval;
	int ret;
	client_thread_args_t client_args;

	/* Setup our defaults. */
	ret = 0;
	retval = NULL;
	m_client = NULL;

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
		/* Client */
		client_args.addr = argv[2];
		client_args.port = (uint16_t)atoi(argv[3]);

		ret = pthread_create(&m_client_thread, NULL, client_thread_func, &client_args);
		if (ret)
			return ret;
		pthread_join(m_client_thread, &retval);
	} else {
		printf("Unkown mode or invalid number of arguments.\n");
		return 1;
	}

cleanup:
	/* Free up any resources. */
	gl_server_free();

	return ret;
}

/**
 * Client thread function.
 *
 * @param args Arguments passed as a pointer to a client_thread_args_t struct.
 *
 * @return Returned value from the whole operation.
 */
void *client_thread_func(void *args) {
	const client_thread_args_t *client_args;
	tcp_err_t err;
	char *tmp;
	char buf[100];
	size_t len;

	/* Get arguments. */
	client_args = (client_thread_args_t *)args;

	/* Get a client handle. */
	m_client = tcp_client_new(client_args->addr, client_args->port);
	if (m_client == NULL)
		return (void *)TCP_ERR_UNKNOWN;

	/* Connect our client to a server. */
	err = tcp_client_connect(m_client);
	if (err)
		return (void *)err;

	/* Print some information about the current state of the connection. */
	tmp = tcp_client_get_ipstr(m_client);
	printf("Client connected to server on %s port %u\n", tmp, client_args->port);

	/* Read incoming data until the connection is closed by the server. */
	while ((err = tcp_client_recv(m_client, buf, 99, &len, false)) == TCP_OK) {
		/* Properly terminate the received string and print it. */
		buf[len] = '\0';
		printf("Data received from %s: \"%s\"\n", tmp, buf);

		/* Echo data back. */
		tcp_client_send(m_client, "Echo: ", 6);
		tcp_client_send(m_client, buf, len);
	}

	/* Check if the connection was closed gracefully. */
	if (err == TCP_EVT_CONN_CLOSED)
		printf("Connection closed by the server at %s:%u\n", tmp, client_args->port);

	/* Disconnect from the server and free up any resources. */
	err = tcp_client_close(m_client);
	tcp_client_free(m_client);
	m_client = NULL;
	printf("Client disconnected from server at %s:%u\n", tmp, client_args->port);

	free(tmp);
	return (void *)err;
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
	printf("Got a Ctrl-C\n");

	/* Stop the client. */
	if (m_client != NULL)
		tcp_client_shutdown(m_client);

	/* Stop the server. */
	gl_server_stop();

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}
