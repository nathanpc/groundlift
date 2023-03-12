/**
 * tcpserver.c
 * TCP server that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "tcpserver.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
#if 0
	struct addrinfo hints;
	struct addrinfo *res;

	/* Setup the socket hints and populate setup structure. */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, port, &hints, &res);

	/* Create a new socket. */
	m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (m_sockfd == -1) {
		perror("server_start@socket");
		return SERVER_ERR_ESOCKET;
	}

	/* Bind ourselves to the address. */
	if (bind(m_sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		perror("server_start@bind");
		return SERVER_ERR_EBIND;
	}
#endif
	struct sockaddr_in sock_addr;

	/* Create a new socket. */
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) {
		perror("server_start@socket");
		return SERVER_ERR_ESOCKET;
	}

	/* Setup the socket address structure for binding. */
	memset(&sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(1234);
	sock_addr.sin_addr.s_addr = (addr == NULL) ? INADDR_ANY : inet_addr(addr);

	/* Bind ourselves to the address. */
	if (bind(m_sock, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in)) == -1) {
		perror("server_start@bind");
		return SERVER_ERR_EBIND;
	}

	return SERVER_OK;
}

/**
 * Stops the server and frees any resources allocated by it.
 */
void server_stop(void) {
	//
}
