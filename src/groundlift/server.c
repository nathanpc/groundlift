/**
 * server.c
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <pthread.h>

#include "defaults.h"
#include "tcp.h"

/* Private variables. */
static server_t *m_server;
static server_conn_t *m_conn;
static pthread_t *m_server_thread;
static pthread_mutex_t *m_server_mutex;
static pthread_mutex_t *m_conn_mutex;

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
	m_conn = NULL;
	m_server_thread = (pthread_t *)malloc(sizeof(pthread_t));

	/* Initialize our mutexes. */
	m_server_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_server_mutex, NULL);
	m_conn_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_conn_mutex, NULL);

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

	/* Free our server mutex. */
	if (m_server_mutex) {
		free(m_server_mutex);
		m_server_mutex = NULL;
	}

	/* Free our connection mutex. */
	if (m_conn_mutex) {
		free(m_conn_mutex);
		m_conn_mutex = NULL;
	}
}

/**
 * Starts the server up.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_thread_join
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
 * @see gl_server_thread_join
 * @see gl_server_free
 * @see gl_server_conn_destroy
 */
bool gl_server_stop(void) {
	tcp_err_t err;

	/* Do we even need to stop it? */
	if (m_server == NULL)
		return true;

	/* Destroy any active connections. */
	pthread_mutex_lock(m_conn_mutex);
	gl_server_conn_destroy();
	pthread_mutex_unlock(m_conn_mutex);

	/* Shut the thing down. */
	pthread_mutex_lock(m_server_mutex);
	err = tcp_server_shutdown(m_server);
	pthread_mutex_unlock(m_server_mutex);

	return err == TCP_OK;
}

/**
 * Closes and frees up any resources associated with the current active
 * connection to the server.
 *
 * @return Return value of tcp_server_conn_close.
 *
 * @see gl_server_stop
 * @see tcp_server_conn_close
 */
tcp_err_t gl_server_conn_destroy(void) {
	tcp_err_t err;

	/* Do we even need to do stuff? */
	if (m_conn == NULL)
		return TCP_OK;

	/* Tell the world that we are closing an active connection. */
	if (m_conn->sockfd != -1)
		printf("Closing the currently active connection\n");

	/* Close the connection and free up any resources allocated. */
	err = tcp_server_conn_shutdown(m_conn);
	tcp_server_conn_free(m_conn);
	m_conn = NULL;

	return err;
}

/**
 * Waits for the server thread to return.
 *
 * @return TRUE if the operation was successful.
 */
bool gl_server_thread_join(void) {
	tcp_err_t *err;
	bool success;

	/* Do we even need to do something? */
	if (m_server_thread == NULL)
		return true;

	/* Join the thread back into us. */
	pthread_join(*m_server_thread, (void **)&err);
	free(m_server_thread);
	m_server_thread = NULL;

	/* Check for success and free our returned value. */
	success = *err <= TCP_OK;
	free(err);

	/* Check if the thread join was successful. */
	return success;
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
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param args No arguments should be passed.
 *
 * @return Pointer to a tcp_err_t with the last error code. This pointer must be
 *         free'd by you.
 */
void *server_thread_func(void *args) {
	tcp_err_t *err;
	char *tmp;

	/* Ignore the argument passed and initialize things. */
	(void)args;
	tmp = NULL;
	err = (tcp_err_t *)malloc(sizeof(tcp_err_t));

	/* Start the server and listen for incoming connections. */
	*err = tcp_server_start(m_server);
	if (*err)
		return (void *)err;

	/* Print some information about the current state of the server. */
	tmp = tcp_server_get_ipstr(m_server);
	printf("Server listening on %s port %u\n", tmp,
		   ntohs(m_server->addr_in.sin_port));
	free(tmp);
	tmp = NULL;

	/* Accept incoming connections. */
	while ((m_conn = tcp_server_conn_accept(m_server)) != NULL) {
		char buf[100];
		size_t len;

		/* Check if the connection socket is valid. */
		if (m_conn->sockfd == -1) {
			gl_server_conn_destroy();
			continue;
		}

		/* Print out some client information. */
		tmp = tcp_server_conn_get_ipstr(m_conn);
		printf("Client at %s connection accepted\n", tmp);

		/* Send some data to the client. */
		*err = tcp_server_conn_send(m_conn, "Hello, world!", 13);
		if (*err)
			goto close_conn;

		/* Read incoming data until the connection is closed by the client. */
		while ((*err = tcp_server_conn_recv(m_conn, buf, 99, &len, false)) == TCP_OK) {
			/* Properly terminate the received string and print it. */
			buf[len] = '\0';
			printf("Data received from %s: \"%s\"\n", tmp, buf);
		}

		/* Check if the connection was closed gracefully. */
		if (*err == TCP_EVT_CONN_CLOSED)
			printf("Connection closed by the client at %s\n", tmp);

	close_conn:
		/* Close the connection and free up any resources. */
		pthread_mutex_lock(m_conn_mutex);
		gl_server_conn_destroy();
		pthread_mutex_unlock(m_conn_mutex);

		/* Inform of the connection being closed. */
		printf("Client %s connection closed\n", tmp);
		free(tmp);
	}

	/* Stop the server and free up any resources. */
	gl_server_stop();
	printf("Server stopped\n");

	return (void *)err;
}
