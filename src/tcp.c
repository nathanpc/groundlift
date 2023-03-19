/**
 * tcp.c
 * TCP server/client that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "tcp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Private definitions. */
#ifndef TCPSERVER_BACKLOG
#define TCPSERVER_BACKLOG 10
#endif /* TCPSERVER_BACKLOG */

/**
 * Creates a brand new server handle object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param addr Address to bind ourselves to or NULL to use INADDR_ANY.
 * @param port Port to listen on.
 *
 * @return Newly allocated server handle object or NULL if we couldn't allocate
 *         the object.
 *
 * @see server_free
 */
server_t *tcp_server_new(const char *addr, uint16_t port) {
	server_t *server;

	/* Allocate some memory for our handle object. */
	server = (server_t *)malloc(sizeof(server_t));
	if (server == NULL)
		return NULL;

	/* Ensure we have a known invalid state for our socket file descriptor. */
	server->sockfd = -1;

	/* Setup the socket address structure for binding. */
	server->addr_in_size = sizeof(struct sockaddr_in);
	memset(&server->addr_in, 0, server->addr_in_size);
	server->addr_in.sin_family = AF_INET;
	server->addr_in.sin_port = htons(port);
	server->addr_in.sin_addr.s_addr =
		(addr == NULL) ? INADDR_ANY : inet_addr(addr);

	return server;
}

/**
 * Frees up any resources allocated by the server.
 *
 * @param server Server handle object to be free'd.
 *
 * @see server_stop
 */
void tcp_server_free(server_t *server) {
	/* Do we even need to do something? */
	if (server == NULL)
		return;

	/* Free the object and NULL it out. */
	free(server);
	server = NULL;
}

/**
 * Starts up the server.
 *
 * @param server Server handle object.
 *
 * @return TCP_OK if the initialization was successful.
 *         TCP_ERR_ESOCKET if the socket function failed.
 *         TCP_ERR_EBIND if the bind function failed.
 *         TCP_ERR_ELISTEN if the listen function failed.
 *
 * @see server_stop
 */
tcp_err_t tcp_server_start(server_t *server) {
	/* Create a new socket file descriptor. */
	server->sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (server->sockfd == -1) {
		perror("server_start@socket");
		return TCP_ERR_ESOCKET;
	}

	/* Bind ourselves to the address. */
	if (bind(server->sockfd, (struct sockaddr *)&server->addr_in,
			 server->addr_in_size) == -1) {
		perror("server_start@bind");
		return TCP_ERR_EBIND;
	}

	/* Start listening on our socket. */
	if (listen(server->sockfd, TCPSERVER_BACKLOG) == -1) {
		perror("server_start@listen");
		return TCP_ERR_ELISTEN;
	}

	return TCP_OK;
}

/**
 * Stops the server but doesn't free the resources allocated.
 *
 * @param server Server handle object.
 *
 * @return TCP_OK if the socket was properly closed.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 *
 * @see server_free
 */
tcp_err_t tcp_server_stop(server_t *server) {
	tcp_err_t err;

	/* Close the socket file descriptor and set it to a known invalid state. */
	err = tcp_socket_close(server->sockfd);
	server->sockfd = -1;

	return err;
}

/**
 * Accepts an incoming connection.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param server    Server handle object.
 *
 * @return Newly allocated server client connection handle object.
 *         NULL in case of an error.
 *
 * @see server_conn_free
 */
server_conn_t *tcp_server_conn_accept(server_t *server) {
	server_conn_t *conn;

	/* Allocate some memory for our handle object. */
	conn = (server_conn_t *)malloc(sizeof(server_conn_t));
	if (conn == NULL)
		return NULL;

	/* Accept the new connection. */
	conn->addr_size = sizeof(struct sockaddr_storage);
	conn->sockfd = accept(server->sockfd, (struct sockaddr *)&conn->addr,
						  &conn->addr_size);

	/* Handle errors. */
	if (conn->sockfd == -1) {
		perror("server_accept@accept");
		return NULL;
	}

	return conn;
}

/**
 * Closes a server client's remote connection.
 *
 * @param conn Server client connection handle object.
 *
 * @return TCP_OK if everything went fine.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 *
 * @see server_conn_free
 */
tcp_err_t tcp_server_conn_close(server_conn_t *conn) {
	tcp_err_t err;

	/* Close the socket file descriptor and set it to a known invalid state. */
	err = tcp_socket_close(conn->sockfd);
	conn->sockfd = -1;

	return err;
}

/**
 * Frees up any resources allocated by a server connection object.
 *
 * @param conn Server client connection handle object to be free'd.
 *
 * @see server_conn_close
 */
void tcp_server_conn_free(server_conn_t *conn) {
	/* Do we even need to do something? */
	if (conn == NULL)
		return;

	/* Free the object and NULL it out. */
	free(conn);
	conn = NULL;
}

/**
 * Closes a socket file descriptor.
 *
 * @param sockfd Socket file descriptor to be closed.
 *
 * @return TCP_OK if the socket was properly closed.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 */
tcp_err_t tcp_socket_close(int sockfd) {
	int ret;

	/* Check if we are even needed. */
	if (sockfd == -1)
		return TCP_OK;

	/* Close the socket file descriptor. */
#ifdef _WIN32
	ret = closesocket(sockfd);
#else
	ret = close(sockfd);
#endif /* _WIN32 */

	/* Check for errors. */
	if (ret == -1) {
		perror("server_close@close");
		return TCP_ERR_ECLOSE;
	}

	return TCP_OK;
}
