/**
 * GroundLift
 * An AirDrop alternative.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defaults.h"
#include "tcp.h"

/* Private variables. */
static char *m_server_addr;
static uint16_t m_server_port;
static bool m_running;

/* Private methods. */
tcp_err_t server(void);
tcp_err_t client(const char *addr, uint16_t port);
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
	/* Setup our defaults. */
	m_running = true;
	m_server_addr = NULL;
	m_server_port = TCPSERVER_PORT;

	/* Catch the interrupt signal from the console. */
	signal(SIGINT, sigint_handler);

	/* Start as server or as client. */
	if ((argc < 2) || (argv[1][0] == 's')) {
		return server();
	} else if ((argc == 4) && (argv[1][0] == 'c')) {
		return client(argv[2], (uint16_t)atoi(argv[3]));
	} else {
		printf("Unkown mode or invalid number of arguments.\n");
		return 1;
	}
}

/**
 * Starts up the server.
 *
 * @return Server return code.
 */
tcp_err_t server(void) {
	tcp_err_t err;
	server_t *server;
	server_conn_t *conn;
	char *tmp;

	/* Get a server handle. */
	server = tcp_server_new(m_server_addr, m_server_port);
	if (server == NULL)
		return TCP_ERR_UNKNOWN;

	/* Start the server and listen for incoming connections. */
	err = tcp_server_start(server);
	if (err)
		return err;

	/* Print some information about the current state of the server. */
	tmp = tcp_server_get_ipstr(server);
	printf("Server listening on %s port %u\n", tmp, m_server_port);
	free(tmp);
	tmp = NULL;

	/* Accept incoming connections. */
	while (m_running && ((conn = tcp_server_conn_accept(server)) != NULL)) {
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
		while (m_running && ((err = tcp_server_conn_recv(conn, buf, 99, &len, false)) == TCP_OK)) {
			/* Properly terminate the received string and print it. */
			buf[len] = '\0';
			printf("Data received from %s: \"%s\"\n", tmp, buf);
		}

		/* Check if the connection was closed gracefully. */
		if (err == TCP_EVT_CONN_CLOSED)
			printf("Connection closed by the client at %s\n", tmp);

close_conn:
		/* Close the connection and free up any resources. */
		tcp_server_conn_close(conn);
		tcp_server_conn_free(conn);

		/* Inform of the connection being closed. */
		printf("Client %s connection closed\n", tmp);
		free(tmp);
}

	/* Stop the server and free up any resources. */
	err = tcp_server_stop(server);
	tcp_server_free(server);
	printf("Server stopped\n");

	return err;
}

/**
 * Connects the client to a server.
 *
 * @return Client return code.
 */
tcp_err_t client(const char *addr, uint16_t port) {
	tcp_err_t err;
	tcp_client_t *client;
	char *tmp;
	char buf[100];
	size_t len;

	/* Get a client handle. */
	client = tcp_client_new(addr, port);
	if (client == NULL)
		return TCP_ERR_UNKNOWN;

	/* Connect our client to a server. */
	err = tcp_client_connect(client);
	if (err)
		return err;

	/* Print some information about the current state of the connection. */
	tmp = tcp_client_get_ipstr(client);
	printf("Client connected to server on %s port %u\n", tmp, port);

	/* Read incoming data until the connection is closed by the server. */
	while (m_running && ((err = tcp_client_recv(client, buf, 99, &len, false)) == TCP_OK)) {
		/* Properly terminate the received string and print it. */
		buf[len] = '\0';
		printf("Data received from %s: \"%s\"\n", tmp, buf);

		/* Echo data back. */
		tcp_client_send(client, "Echo: ", 6);
		tcp_client_send(client, buf, len);

		/* Just close our connection. */
		m_running = false;
	}

	/* Check if the connection was closed gracefully. */
	if (err == TCP_EVT_CONN_CLOSED)
		printf("Connection closed by the server at %s:%u\n", tmp, port);

	/* Disconnect from the server and free up any resources. */
	err = tcp_client_close(client);
	tcp_client_free(client);
	printf("Client disconnected from server at %s:%u\n", tmp, port);

	free(tmp);
	return err;
}

/**
 * Handles the SIGINT interrupt event.
 *
 * @param sig Signal handle that generated this interrupt.
 */
void sigint_handler(int sig) {
	/* Clear our flag and ignore the interrupt. Pray for a graceful exit. */
	m_running = false;
	signal(sig, SIG_IGN);
}
