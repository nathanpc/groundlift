/**
 * client.c
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <groundlift/defaults.h>
#include <groundlift/tcp.h>
#include <pthread.h>

/* Private variables. */
static tcp_client_t *m_client;
static pthread_mutex_t *m_client_mutex;
static pthread_t *m_client_thread;

/* Private methods. */
void *client_thread_func(void *args);

/**
 * Initializes everything related to the client to a known clean state.
 *
 * @param addr Address to connect to.
 * @param port Port to connect to.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_connect
 */
bool gl_client_init(const char *addr, uint16_t port) {
	/* Ensure we have everything in a known clean state. */
	m_client_thread = (pthread_t *)malloc(sizeof(pthread_t));

	/* Initialize our mutex. */
	m_client_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_client_mutex, NULL);

	/* Get a server handle. */
	m_client = tcp_client_new(addr, port);
	return m_client != NULL;
}

/**
 * Frees up any resources allocated by the client. Shutting down the connection
 * prior to calling this function isn't required.
 *
 * @see gl_client_disconnect
 */
void gl_client_free(void) {
	/* Disconnect the client and free up any allocated resources. */
	gl_client_disconnect();
	tcp_client_free(m_client);
	m_client = NULL;

	/* Join the client thread. */
	gl_client_thread_join();

	/* Free our mutex. */
	if (m_client_mutex) {
		free(m_client_mutex);
		m_client_mutex = NULL;
	}
}

/**
 * Connects the client to a server.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_loop
 * @see gl_client_disconnect
 */
bool gl_client_connect(void) {
	int ret;

	/* Create the client thread. */
	ret = pthread_create(m_client_thread, NULL, client_thread_func, NULL);
	if (ret)
		m_client_thread = NULL;

	return ret == 0;
}

/**
 * Disconnects the client if needed.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_loop
 * @see gl_client_connect
 */
bool gl_client_disconnect(void) {
	tcp_err_t err;

	/* Do we even need to shut it down? */
	if (m_client == NULL)
		return true;

	/* Shut the connection down. */
	pthread_mutex_lock(m_client_mutex);
	err = tcp_client_shutdown(m_client);
	pthread_mutex_unlock(m_client_mutex);

	return err == TCP_OK;
}

/**
 * Waits for the client thread to return.
 *
 * @return TRUE if the operation was successful.
 */
bool gl_client_thread_join(void) {
	tcp_err_t *err;
	bool success;

	/* Do we even need to do something? */
	if (m_client_thread == NULL)
		return true;

	/* Join the thread back into us. */
	pthread_join(*m_client_thread, (void **)&err);
	free(m_client_thread);
	m_client_thread = NULL;

	/* Check for success and free our returned value. */
	success = *err <= TCP_OK;
	free(err);

	/* Check if the thread join was successful. */
	return success;
}

/**
 * Gets the client object handle.
 *
 * @return Client object handle.
 */
tcp_client_t *gl_client_get(void) {
	return m_client;
}

/**
 * Client's thread function.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param args No arguments should be passed.
 *
 * @return Pointer to a tcp_err_t with the last error code. This pointer must be
 *         free'd by you.
 */
void *client_thread_func(void *args) {
	tcp_err_t *err;
	char *tmp;
	char buf[100];
	size_t len;

	/* Ignore the argument passed. */
	(void)args;
	tmp = NULL;
	err = (tcp_err_t *)malloc(sizeof(tcp_err_t));

	/* Get a client handle. */
	if (m_client == NULL) {
		fprintf(stderr, "server_thread_func: Client handle is NULL.\n");

		*err = TCP_ERR_UNKNOWN;
		return (void *)err;
	}

	/* Connect our client to a server. */
	*err = tcp_client_connect(m_client);
	if (*err)
		return (void *)err;

	/* Print some information about the current state of the connection. */
	tmp = tcp_client_get_ipstr(m_client);
	printf("Client connected to server on %s port %u\n", tmp,
		   ntohs(m_client->addr_in.sin_port));

	/* Read incoming data until the connection is closed by the server. */
	while ((*err = tcp_client_recv(m_client, buf, 99, &len, false)) == TCP_OK) {
		/* Properly terminate the received string and print it. */
		buf[len] = '\0';
		printf("Data received from %s: \"%s\"\n", tmp, buf);

		/* Echo data back. */
		tcp_client_send(m_client, "Echo: ", 6);
		tcp_client_send(m_client, buf, len);
	}

	/* Check if the connection was closed gracefully. */
	if (*err == TCP_EVT_CONN_CLOSED) {
		printf("Connection closed by the server at %s:%u\n", tmp,
			   ntohs(m_client->addr_in.sin_port));
	}

	/* Disconnect from the server and free up any resources. */
	gl_client_disconnect();
	printf("Client disconnected from server at %s\n", tmp);

	/* Ensure temp buffer is free'd. */
	if (tmp)
		free(tmp);

	return (void *)err;
}
