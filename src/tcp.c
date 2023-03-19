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
#include <sys/_types/_ssize_t.h>
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
server_conn_t *tcp_server_conn_accept(const server_t *server) {
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
 * Send some data to a client connected to our server.
 *
 * @param conn Server client connection handle object.
 * @param buf  Data to be sent.
 * @param len  Length of the data to be sent.
 *
 * @return TCP_OK if the operation was successful.
 *         TCP_ERR_ESEND if the send function failed.
 *
 * @see tcp_socket_send
 */
tcp_err_t tcp_server_conn_send(const server_conn_t *conn, const void *buf, size_t len) {
	return tcp_socket_send(conn->sockfd, buf, len, NULL);
}

/**
 * Receives some data from a client connected to our server.
 *
 * @param conn     Server client connection handle object.
 * @param buf      Buffer to store the received data.
 * @param buf_len  Length of the buffer to store the data.
 * @param recv_len Pointer to store the number of bytes actually received. Will
 *                 be ignored if NULL is passed.
 * @param peek     Should we just peek at the data to be received?
 *
 * @return TCP_OK if the operation was successful.
 *         TCP_ERR_ERECV if the recv function failed.
 *
 * @see tcp_socket_recv
 */
tcp_err_t tcp_server_conn_recv(const server_conn_t *conn, void *buf, size_t buf_len, size_t *recv_len, bool peek) {
	return tcp_socket_recv(conn->sockfd, buf, buf_len, recv_len, peek);
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
 * Sends some data to a socket file descriptor.
 *
 * @param sockfd   Socket file descriptor.
 * @param buf      Data to be sent.
 * @param len      Length of the data to be sent.
 * @param sent_len Pointer to store the number of bytes actually sent. Ignored
 *                 if NULL is passed.
 *
 * @return TCP_OK if the operation was successful.
 *         TCP_ERR_ESEND if the send function failed.
 *
 * @see send
 */
tcp_err_t tcp_socket_send(int sockfd, const void *buf, size_t len, size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = send(sockfd, buf, len, 0);
	if (bytes_sent == -1) {
		perror("tcp_socket_send@send");
		return TCP_ERR_ESEND;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return TCP_OK;
}

/**
 * Receive some data from a socket file descriptor.
 *
 * @param sockfd   Socket file descriptor.
 * @param buf      Buffer to store the received data.
 * @param buf_len  Length of the buffer to store the data.
 * @param recv_len Pointer to store the number of bytes actually received. Will
 *                 be ignored if NULL is passed.
 * @param peek     Should we just peek at the data to be received?
 *
 * @return TCP_OK if we received some data.
 *         TCP_EVT_CONN_CLOSED if the connection was closed by the client.
 *         TCP_ERR_ERECV if the recv function failed.
 *
 * @see recv
 */
tcp_err_t tcp_socket_recv(int sockfd, void *buf, size_t buf_len, size_t *recv_len, bool peek) {
	ssize_t bytes_recv;

	/* Try to read some information from a socket. */
	bytes_recv = recv(sockfd, buf, buf_len, (peek) ? MSG_PEEK : 0);
	if (bytes_recv == -1) {
		perror("tcp_socket_recv@recv");
		return TCP_ERR_ERECV;
	}

	/* Return the number of bytes sent. */
	if (recv_len != NULL)
		*recv_len = bytes_recv;

	/* Check if the connection was closed gracefully by the client. */
	if (bytes_recv == 0)
		return TCP_EVT_CONN_CLOSED;

	return TCP_OK;
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
