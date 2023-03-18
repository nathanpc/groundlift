/**
 * tcpserver.c
 * TCP server that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "tcpserver.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Private definitions. */
#ifndef TCPSERVER_BACKLOG
#define TCPSERVER_BACKLOG 10
#endif /* TCPSERVER_BACKLOG */

/* Private variables. */
static int m_sock;

/**
 * Starts up the server listening on the specified port.
 *
 * @param addr Address to bind ourselves to or NULL to use INADDR_ANY.
 * @param port Port to listen on.
 *
 * @return SERVER_OK if the initialization was successful.
 */
server_err_t server_start(const char *addr, uint16_t port) {
	struct sockaddr_in sock_addr;

	/* Create a new socket file descriptor. */
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) {
		perror("server_start@socket");
		return SERVER_ERR_ESOCKET;
	}

	/* Setup the socket address structure for binding. */
	memset(&sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = (addr == NULL) ? INADDR_ANY : inet_addr(addr);

	/* Bind ourselves to the address. */
	if (bind(m_sock, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in)) == -1) {
		perror("server_start@bind");
		return SERVER_ERR_EBIND;
	}

	/* Start listening on our socket. */
	if (listen(m_sock, TCPSERVER_BACKLOG) == -1) {
		perror("server_start@listen");
		return SERVER_ERR_ELISTEN;
	}

	return SERVER_OK;
}

/**
 * Accepts an incoming connection.
 *
 * @param conn_addr Pointer to the struct that will hold the incoming connection
 *					information.
 *
 * @return Accepted connection socket file descriptor or -1 in case of an error.
 */
int server_accept(struct sockaddr_storage *conn_addr) {
	int conn_sockfd;
	socklen_t addr_len;

	/* Accept the new connection. */
	addr_len = sizeof(struct sockaddr_storage);
	conn_sockfd = accept(m_sock, (struct sockaddr *)conn_addr, &addr_len);

	/* Handle errors. */
	if (conn_sockfd == -1) {
		perror("server_accept@accept");
		return -1;
	}

	return conn_sockfd;
}

/**
 * Closes a socket file descriptor and frees any resources allocated by it.
 *
 * @param sockfd Socket file descriptor to be closed.
 *
 * @return SERVER_OK if the socket was properly closed.
 *		   SERVER_ERR_ECLOSE if the socket failed to close properly.
 */
server_err_t server_close(int sockfd) {
	int ret;

	/* Close the socket file descriptor. */
#ifdef _WIN32
	ret = closesocket(sockfd);
#else
	ret = close(sockfd);
#endif /* _WIN32 */

	/* Check for errors. */
	if (ret == -1) {
		perror("server_close@close");
		return SERVER_ERR_ECLOSE;
	}

	return SERVER_OK;
}

/**
 * Stops the server and frees any resources allocated by it.
 *
 * @return SERVER_OK if the socket was properly closed.
 *		   SERVER_ERR_ECLOSE if the socket failed to close properly.
 */
server_err_t server_stop(void) {
	return server_close(m_sock);
}
