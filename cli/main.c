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

/* Client thread function arguments structure definition. */
typedef struct {
	const char *addr;
	uint16_t port;
} client_thread_args_t;

/* Private variables. */
static char *m_server_addr;
static uint16_t m_server_port;
static server_t *m_server;
static tcp_client_t *m_client;
static pthread_t m_server_thread;
static pthread_t m_client_thread;

/* Private methods. */
void sigint_handler(int sig);
void *server_thread_func(void *arg);
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
	retval = NULL;
	m_server = NULL;
	m_client = NULL;
	m_server_addr = NULL;
	m_server_port = TCPSERVER_PORT;

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Start as server or as client. */
	if ((argc < 2) || (argv[1][0] == 's')) {
		/* Server */
		ret = pthread_create(&m_server_thread, NULL, server_thread_func, NULL);
		if (ret)
			return ret;
		pthread_join(m_server_thread, &retval);
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

	return (int)retval;
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
	printf("Client disconnected from server at %s:%u\n", tmp, client_args->port);

	free(tmp);
	return (void *)err;
}

/**
 * Server thread function.
 *
 * @param arg No arguments should be passed.
 *
 * @return Returned value from the whole operation.
 */
void *server_thread_func(void *arg) {
	tcp_err_t err;
	server_conn_t *conn;
	char *tmp;

	/* Ignore the argument passed. */
	(void)arg;

	/* Get a server handle. */
	m_server = tcp_server_new(m_server_addr, m_server_port);
	if (m_server == NULL)
		return (void *)TCP_ERR_UNKNOWN;

	/* Start the server and listen for incoming connections. */
	err = tcp_server_start(m_server);
	if (err)
		return (void *)err;

	/* Print some information about the current state of the server. */
	tmp = tcp_server_get_ipstr(m_server);
	printf("Server listening on %s port %u\n", tmp, m_server_port);
	free(tmp);
	tmp = NULL;

	/* Accept incoming connections. */
	while ((conn = tcp_server_conn_accept(m_server)) != NULL) {
		char buf[100];
		size_t len;

		/* Print out some client information. */
		tmp = tcp_server_conn_get_ipstr(conn);
		printf("Client at %s connection accepted\n", tmp);

		/* Send some data to the client. */
		err = tcp_server_conn_send(conn, "Hello, world!", 13);
		if (err)
			goto close_conn;

		/* Read incoming data until the connection is closed by the client. */
		while ((err = tcp_server_conn_recv(conn, buf, 99, &len, false)) == TCP_OK) {
			/* Properly terminate the received string and print it. */
			buf[len] = '\0';
			printf("Data received from %s: \"%s\"\n", tmp, buf);
		}

		/* Check if the connection was closed gracefully. */
		if (err == TCP_EVT_CONN_CLOSED)
			printf("Connection closed by the client at %s\n", tmp);

	close_conn:
		/* Close the connection and free up any resources. */
		err = tcp_server_conn_close(conn);
		if (err)
			return (void *)err;
		tcp_server_conn_free(conn);

		/* Inform of the connection being closed. */
		printf("Client %s connection closed\n", tmp);
		free(tmp);
	}

	/* Stop the server and free up any resources. */
	err = tcp_server_stop(m_server);
	tcp_server_free(m_server);
	printf("Server stopped\n");

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
	if (m_server != NULL)
		tcp_server_shutdown(m_server);

	/* Don't let the signal propagate. */
	signal(sig, SIG_IGN);
}
