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
#include "utils.h"

/* Private definitions. */
#define LISTEN_BACKLOG      5  /* Server socket listening backlog. */
#define SERVER_TIMEOUT_SECS 1  /* Timeout of server communications in seconds. */

/**
 * Initializes the sockets API.
 * 
 * @return TRUE if the initialization was successful, FALSE otherwise.
 */
bool socket_init(void) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;

	/* Initialize the Winsock stuff. */
	wVersionRequested = MAKEWORD(2, 2);
	if ((ret = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		log_printf(LOG_CRIT, "WSAStartup failed with error %d", ret);
		return false;
	}
#endif /* _WIN32 */

	return true;
}

/**
 * Populates the IP address structure.
 *
 * @param sa      Address storage structure to be populated.
 * @param af      Address family to be guessed.
 * @param addrlen Pointer to store the length of the address structure.
 * @param addr    IP address to be populated.
 * @param port    Port to be populated.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool socket_addr_setup(struct sockaddr_storage *sa, int *af, socklen_t *addrlen,
                       const char *addr, const char *port) {
#ifdef WITHOUT_GETADDRINFO
	long portnum;

	/* Convert port to IP port integer. */
	if (!parse_num(port, &portnum)) {
		log_syserr(LOG_ERROR, "Failed to parse port %s into a number", port);
		return false;
	}

	/* Guess the address family. */
	*af = (strchr(addr, ':') == NULL) ? AF_INET : AF_INET6;

	/* Zero out address structure and cache its size. */
	memset(sa, '\0', sizeof(struct sockaddr_storage));
	*addrlen = *af == AF_INET ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);

	/* Populate the IP address structure. */
	if (*af == AF_INET) {
		struct sockaddr_in *inaddr = (struct sockaddr_in*)sa;
		inaddr->sin_family = *af;
		inaddr->sin_port = htons((uint16_t)portnum);
		inaddr->sin_addr.s_addr = inet_addr(addr);
	} else {
		struct sockaddr_in6 *in6addr = (struct sockaddr_in6*)sa;
		in6addr->sin6_family = *af;
		in6addr->sin6_port = htons((uint16_t)portnum);
		log_printf(LOG_CRIT, "IPv6 not yet implemented.");
		return false;
	}
#else
	int status;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *ai;

	/* Populate getaddrinfo hints. */
	memset(sa, '\0', sizeof(struct sockaddr_storage));
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* Get address information. */
	if ((status = getaddrinfo(addr, port, &hints, &res)) != 0) {
		log_printf(LOG_ERROR, "Failed to get address information for %s: %s",
			addr, gai_strerror(status));
		return false;
	}

	/* Check if we got anything. */
	if (res == NULL) {
		log_printf(LOG_ERROR, "No address information found for %s", addr);
		return false;
	}

	/* Select which address information object to use (prefer IPv4). */
	ai = res;
	while (ai != NULL) {
		/* Select IPv4 if one is available. */
		if (ai->ai_family == AF_INET)
			break;

		/* Go to the next result. */
		ai = ai->ai_next;
	}

	/* Pick the first result if none were previously selected. */
	if (ai == NULL)
		ai = res;

	/* Populate our address structure. */
	*af = ai->ai_family;
	*addrlen = ai->ai_addrlen;
	memcpy(sa, ai->ai_addr, ai->ai_addrlen);

	/* Clean up the results linked list. */
	freeaddrinfo(res);
#endif /* WITHOUT_GETADDRINFO */

	return true;
}

/**
 * Opens up a new listening socket for server operation.
 *
 * @param addr IP address to bind ourselves to.
 * @param port Port to bind ourselves to.
 *
 * @return Socket file descriptor or SOCKERR if an error occurred.
 */
sockfd_t socket_new_server(const char *addr, const char *port) {
	struct sockaddr_storage sa;
	sockfd_t sockfd;
	socklen_t addrlen;
	int af;
#ifdef _WIN32
	DWORD dwTimeout;
	char flag;
#else
	struct timeval tv;
	int flag;
#endif /* _WIN32 */

	/* Populate socket address information. */
	if (!socket_addr_setup(&sa, &af, &addrlen, addr, port))
		return SOCKERR;

	/* Get a socket file descriptor. */
	sockfd = socket(af == AF_INET ? PF_INET : PF_INET6, SOCK_STREAM, 0);
	if (sockfd == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to get a server socket file descriptor");
		return SOCKERR;
	}

	/* Ensure we don't have to worry about address already in use errors. */
	flag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag,
			sizeof(flag)) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to set server socket address reuse");
		socket_close(sockfd, false);
		return SOCKERR;
	}

	/* Set a reception timeout so that we don't block indefinitely. */
#ifdef _WIN32
	dwTimeout = SERVER_TIMEOUT_SECS * 1000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeout,
			sizeof(dwTimeout)) == SOCKERR) {
#else
	tv.tv_sec = SERVER_TIMEOUT_SECS;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
			sizeof(tv)) == SOCKERR) {
#endif /* _WIN32 */
		log_sockerr(LOG_CRIT, "Failed to set server socket receive timeout");
		socket_close(sockfd, false);
		return SOCKERR;
	}

	/* Bind address to socket. */
	if (bind(sockfd, (struct sockaddr*)&sa, addrlen) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed binding to server socket");
		socket_close(sockfd, false);
		return SOCKERR;
	}

	/* Start listening on our desired socket. */
	if (listen(sockfd, LISTEN_BACKLOG) == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to listen on server socket");
		socket_close(sockfd, false);
		return SOCKERR;
	}

	log_printf(LOG_INFO, "Server running on %s:%s", addr, port);
	return sockfd;
}

/**
 * Opens up a new TCP connecting socket for client operation.
 *
 * @param addr IP address of the server to connect to.
 * @param port Port that the server is listening on.
 *
 * @return Socket file descriptor or SOCKERR if an error occurred.
 */
sockfd_t socket_new_client(const char *addr, const char *port) {
	struct sockaddr_storage sa;
	sockfd_t sockfd;
	socklen_t addrlen;
	int af;

	/* Populate socket address information. */
	if (!socket_addr_setup(&sa, &af, &addrlen, addr, port))
		return SOCKERR;

	/* Get a socket file descriptor. */
	sockfd = socket(af == AF_INET ? PF_INET : PF_INET6, SOCK_STREAM, 0);
	if (sockfd == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to get a server socket file descriptor");
		return SOCKERR;
	}

	/* Connect to the server. */
	if (connect(sockfd, (struct sockaddr *)&sa, addrlen) == SOCKERR) {
		log_sockerr(LOG_ERROR, "Failed to connect to server %s:%s", addr, port);
		sockclose(sockfd);
		return SOCKERR;
	}

	return sockfd;
}

/**
 * Closes a socket and optionally shut it down beforehand.
 *
 * @warning This function won't call close() if shutdown() fails.
 *
 * @param sockfd Socket to be closed.
 * @param shut   Should we call shutdown before closing?
 *
 * @return 0 if the operation was successful, SOCKERR in case of an error.
 */
int socket_close(sockfd_t sockfd, bool shut) {
	/* Check if we are even needed. */
	if (sockfd == SOCKERR)
		return 0;

	/* Shutdown the socket file descriptor. */
	if (shut) {
#ifdef _WIN32
		if (shutdown(sockfd, SD_BOTH) == SOCKERR) {
			if (sockerrno != WSAENOTCONN) {
				log_sockerr(LOG_ERROR, "Failed to shutdown socket");
				return SOCKERR;
			} else {
#ifdef _DEBUG
				log_sockerr(LOG_NOTICE, "Suppressed socket shutdown error");
#endif /* _DEBUG */
			}
		}
#else
		if (shutdown(sockfd, SHUT_RDWR) == SOCKERR) {
			if ((errno != ENOTCONN) && (errno != EINVAL)) {
				log_sockerr(LOG_ERROR, "Failed to shutdown socket");
				return SOCKERR;
			} else {
#ifdef _DEBUG
				log_sockerr(LOG_NOTICE, "Suppressed socket shutdown error");
#endif /* _DEBUG */
			}
		}
#endif /* _WIN32 */
	}

	/* Close the socket file descriptor and return. */
	return sockclose(sockfd);
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
#ifdef WITHOUT_INET_NTOP
	if (af == AF_INET) {
		return strcpy(buf, inet_ntoa(((struct sockaddr_in*)addr)->sin_addr));
	} else {
		log_printf(LOG_ERROR, "IPv6 not yet implemented in inet_addr_str");
		return NULL;
	}
#else
	if (af == AF_INET) {
		return inet_ntop(af, &((struct sockaddr_in*)addr)->sin_addr, buf,
		                 INET6_ADDRSTRLEN);
	} else {
		return inet_ntop(af, &((struct sockaddr_in6*)addr)->sin6_addr, buf,
		                 INET6_ADDRSTRLEN);
	}
#endif /* WITHOUT_INET_NTOP */
}
