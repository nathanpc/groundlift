/**
 * server.c
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <groundlift/defaults.h>
#include <groundlift/tcp.h>
#include <pthread.h>

/* Private variables. */
static server_t *m_server;
static pthread_t *m_server_thread;

/* Private methods. */
void *server_thread_func(void *args);

/**
 * Initializes everything related to the server to a known clean state.
 *
 * @param addr Address to listen on.
 * @param port Port to listen on.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_start
 */
bool gl_server_init(const char *addr, uint16_t port) {
	/* Ensure we have everything in a known clean state. */
	m_server_thread = (pthread_t *)malloc(sizeof(pthread_t));

	/* Get a server handle. */
	m_server = tcp_server_new(addr, port);
	return m_server != NULL;
}

/**
 * Frees up any resources allocated by the server. Shutting down the server
 * previously to calling this function isn't required.
 *
 * @see gl_server_stop
 */
void gl_server_free(void) {
	/* Stop the server and free up any allocated resources. */
	gl_server_stop();
	tcp_server_free(m_server);
	m_server = NULL;

	/* Join the server thread. */
	gl_server_thread_join();
}

/**
 * Starts the server up.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_loop
 * @see gl_server_stop
 */
bool gl_server_start(void) {
	int ret;

	/* Create the server thread. */
	ret = pthread_create(m_server_thread, NULL, server_thread_func, NULL);
	if (ret)
		m_server_thread = NULL;

	return ret == 0;
}

/**
 * Stops the running server if needed.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_loop
 * @see gl_server_free
 */
bool gl_server_stop(void) {
	/* Do we even need to stop it? */
	if (m_server == NULL)
		return true;

	/* Shut the thing down. */
	return tcp_server_shutdown(m_server) == TCP_OK;
}

/**
 * Waits for the server thread to return.
 *
 * @return TRUE if the operation was successful.
 */
bool gl_server_thread_join(void) {
	void *retval;

	/* Do we even need to do something? */
	if (m_server_thread == NULL)
		return true;

	/* Join the thread back into us. */
	pthread_join(*m_server_thread, &retval);
	free(m_server_thread);
	m_server_thread = NULL;

	/* Check if the thread join was successful. */
	return retval == 0;
}

/**
 * Gets the server object handle.
 *
 * @return Server object handle.
 */
server_t *gl_server_get(void) {
	return m_server;
}

/**
 * Server thread function.
 *
 * @param args No arguments should be passed.
 *
 * @return Returned value from the whole operation.
 */
void *server_thread_func(void *args) {
	tcp_err_t err;
	server_conn_t *conn;
	char *tmp;

	/* Ignore the argument passed. */
	(void)args;
	tmp = NULL;
	conn = NULL;

	/* Start the server and listen for incoming connections. */
	err = tcp_server_start(m_server);
	if (err)
		return false;

	/* Print some information about the current state of the server. */
	tmp = tcp_server_get_ipstr(m_server);
	printf("Server listening on %s port %u\n", tmp,
		   ntohs(m_server->addr_in.sin_port));
	free(tmp);
	tmp = NULL;

	/* Accept incoming connections. */
	while ((conn = tcp_server_conn_accept(m_server)) != NULL) {
		char buf[100];
		size_t len;

		/* Check if the connection socket is valid. */
		if (conn->sockfd == -1) {
			tcp_server_conn_free(conn);
			conn = NULL;

			continue;
		}

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
		conn = NULL;

		/* Inform of the connection being closed. */
		printf("Client %s connection closed\n", tmp);
		free(tmp);
	}

	/* Stop the server and free up any resources. */
	gl_server_stop();
	printf("Server stopped\n");

	return (void *)err;
}
