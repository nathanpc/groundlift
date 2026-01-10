/**
* sockets.c
* Platform-independent abstraction layer over socket operations.
*
* @author Nathan Campos <nathan@innoveworkshop.com>
*/

#include "sockets.h"

#include <stdlib.h>
#include <string.h>

#include "logging.h"

/* Private definitions. */
#define LISTEN_BACKLOG      5  /* Server socket listening backlog. */
#define SERVER_TIMEOUT_SECS 1  /* Timeout of server communications in seconds. */

/**
 * Initializes the sockets API.
 */
void socket_init(void) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	/* Initialize the Winsock stuff. */
	wVersionRequested = MAKEWORD(2, 2);
	if ((ret = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		log_printf(LOG_CRIT, "WSAStartup failed with error %d", ret);
		return 1;
	}
#endif /* _WIN32 */
}

/**
 * Opens up a new listening socket for server operation.
 *
 * @param af   Address family.
 * @param addr IP address to bind ourselves to.
 * @param port Port to bind ourselves to.
 *
 * @return Socket file descriptor or SOCKERR if an error occurred.
 */
sockfd_t socket_new_server(int af, const char *addr, uint16_t port) {
	struct sockaddr_storage sa;
	sockfd_t sockfd;
	socklen_t addrlen;
	struct timeval tv;
#ifdef _WIN32
	char flag;
#else
	int flag;
#endif /* _WIN32 */

	/* Zero out address structure and cache its size. */
	memset(&sa, '\0', sizeof(sa));
	addrlen = af == AF_INET ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);

	/* Get a socket file descriptor. */
	sockfd = socket(af == AF_INET ? PF_INET : PF_INET6, SOCK_STREAM, 0);
	if (sockfd == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to get a server socket file descriptor");
		return SOCKERR;
	}

	/* Populate socket address information. */
	if (af == AF_INET) {
		struct sockaddr_in *inaddr = (struct sockaddr_in*)&sa;
		inaddr->sin_family = af;
		inaddr->sin_port = htons(port);
		inaddr->sin_addr.s_addr = inet_addr(addr);
	} else {
		struct sockaddr_in6 *in6addr = (struct sockaddr_in6*)&sa;
		in6addr->sin6_family = af;
		in6addr->sin6_port = htons(port);
		log_printf(LOG_CRIT, "IPv6 not yet implemented.");
		return SOCKERR;
	}

	/* Ensure we don't have to worry about address already in use errors. */
	flag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag,
			sizeof(flag)) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to set server socket address reuse");
		sockclose(sockfd);
		return SOCKERR;
	}

	/* Set a reception timeout so that we don't block indefinitely. */
	tv.tv_sec = SERVER_TIMEOUT_SECS;
	tv.tv_usec = 0;
#ifdef _WIN32
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,
			sizeof(tv)) == SOCKERR) {
#else
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
			sizeof(tv)) == SOCKERR) {
#endif /* _WIN32 */
		log_sockerr(LOG_CRIT, "Failed to set server socket receive timeout");
		sockclose(sockfd);
		return SOCKERR;
	}

	/* Bind address to socket. */
	if (bind(sockfd, (struct sockaddr*)&sa, addrlen) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed binding to server socket");
		sockclose(sockfd);
		return SOCKERR;
	}

	/* Start listening on our desired socket. */
	if (listen(sockfd, LISTEN_BACKLOG) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to listen on server socket");
		sockclose(sockfd);
		return SOCKERR;
	}

	log_printf(LOG_INFO, "Server running on %s:%u", addr, port);
	return sockfd;
}

/**
 * Gets a string representation of a network address structure.
 *
 * @param af   Socket address family.
 * @param addr Network address structure.
 * @param buf  Destination string, pre-allocated to hold an IPv6 address.
 *
 * @return Pointer to the destination string or NULL if an error occurred.
 */
const char* inet_addr_str(int af, void *addr, char *buf) {
#ifdef WITH_INET_NTOP
	if (af == AF_INET) {
		return inet_ntop(af, &((struct sockaddr_in*)addr)->sin_addr, buf,
		                 INET6_ADDRSTRLEN);
	} else {
		return inet_ntop(af, &((struct sockaddr_in6*)addr)->sin6_addr, buf,
		                 INET6_ADDRSTRLEN);
	}
#else
	if (af == AF_INET) {
		return strcpy(buf, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr));
	} else {
		log_printf(LOG_ERROR, "IPv6 not yet implemented in inet_addr_str");
		return NULL;
	}
#endif /* WITH_INET_NTOP */
}
