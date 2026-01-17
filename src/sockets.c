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
 * Sets up a brand new socket handle and its associated variables.
 *
 * @param af      Address family.
 * @param sa      Address storage structure to be initialized.
 * @param addrlen Pointer to store the length of the address structure.
 *
 * @return Initialized socket file descriptor or SOCKERR if an error occurred.
 */
sockfd_t socket_new(int af, struct sockaddr_storage *sa, socklen_t *addrlen) {
	/* Zero out address structure and cache its size. */
	memset(sa, '\0', sizeof(struct sockaddr_storage));
	*addrlen = af == AF_INET ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);

	/* Get a new socket file descriptor. */
	return socket(af == AF_INET ? PF_INET : PF_INET6, SOCK_STREAM, 0);
}

/**
 * Populates the IP address structure.
 *
 * @param sa   Address storage structure to be populated.
 * @param af   Address family.
 * @param addr IP address to be populated.
 * @param port Port to be populated.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
bool socket_addr_setup(struct sockaddr_storage *sa, int af, const char *addr,
                       uint16_t port) {
	if (af == AF_INET) {
		struct sockaddr_in *inaddr = (struct sockaddr_in*)sa;
		inaddr->sin_family = af;
		inaddr->sin_port = htons(port);
		inaddr->sin_addr.s_addr = inet_addr(addr);
	} else {
		struct sockaddr_in6 *in6addr = (struct sockaddr_in6*)sa;
		in6addr->sin6_family = af;
		in6addr->sin6_port = htons(port);
		log_printf(LOG_CRIT, "IPv6 not yet implemented.");
		return false;
	}

	return true;
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

	/* Get a socket file descriptor. */
	sockfd = socket_new(af, &sa, &addrlen);
	if (sockfd == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to get a server socket file descriptor");
		return SOCKERR;
	}

	/* Populate socket address information. */
	if (!socket_addr_setup(&sa, af, addr, port)) {
		socket_close(sockfd, false);
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

	log_printf(LOG_INFO, "Server running on %s:%u", addr, port);
	return sockfd;
}

/**
 * Opens up a new TCP connecting socket for client operation.
 *
 * @param af   Address family.
 * @param addr IP address of the server to connect to.
 * @param port Port that the server is listening on.
 *
 * @return Socket file descriptor or SOCKERR if an error occurred.
 */
sockfd_t socket_new_client(int af, const char *addr, uint16_t port) {
	struct sockaddr_storage sa;
	sockfd_t sockfd;
	socklen_t addrlen;

	/* Get a socket file descriptor. */
	sockfd = socket_new(af, &sa, &addrlen);
	if (sockfd == SOCKERR) {
		log_sockerr(LOG_CRIT, "Failed to get a server socket file descriptor");
		return SOCKERR;
	}

	/* Populate socket address information. */
	if (!socket_addr_setup(&sa, af, addr, port)) {
		sockclose(sockfd);
		return SOCKERR;
	}

	/* Connect to the server. */
	if (connect(sockfd, (struct sockaddr *)&sa, addrlen) == SOCKERR) {
		log_sockerr(LOG_ERROR, "Failed to connect to server %s:%u", addr, port);
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
