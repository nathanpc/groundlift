/**
 * sockets.c
 * Platform-independent abstraction layer over the sockets API.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/bits.h>
#include <utils/logging.h>
#ifdef _WIN32
	#include <utils/utf16.h>
#else
	#ifndef SINGLE_IFACE_MODE
		#include <ifaddrs.h>
		#include <netinet/in.h>
		#include <net/if.h>
	#endif /* !SINGLE_IFACE_MODE */

	#include <sys/errno.h>
	#include <sys/time.h>
	#include <unistd.h>
#endif /* _WIN32 */

#include "defaults.h"
#include "error.h"

/**
 * Creates a brand new socket handle object.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated socket handle object or NULL if we couldn't allocate
 *         the object.
 *
 * @see socket_setup
 * @see socket_setup_inaddr
 * @see socket_free
 */
sock_bundle_t *socket_new() {
	sock_bundle_t *sock;

	/* Allocate some memory for our handle object. */
	sock = (sock_bundle_t *)malloc(sizeof(sock_bundle_t));
	if (sock == NULL)
		return NULL;

	/* Ensure we have a known invalid state. */
	sock->fd = INVALID_SOCKET;
	sock->addr = NULL;
	sock->addr_len = 0;

	return sock;
}

/**
 * Creates a full copy of a socket handle object.
 *
 * @warning This function allocates memory that must be free'd by you.
 * @warning A new socket descriptor isn't created, so be careful if you're
 *          relying on it or using it in any way since it may become invalid at
 *          any moment.
 *
 * @param sock Socket handle object to be duplicated.
 *
 * @return Duplicate socket handle object or NULL if an error occurred.
 *
 * @see socket_free
 */
sock_bundle_t *socket_dup(const sock_bundle_t *sock) {
	sock_bundle_t *dup = socket_new();

	/* Populate the socket bundle object. */
	dup->fd = sock->fd;
	dup->addr = sock->addr;
	dup->addr_len = sock->addr_len;

	return dup;
}

/**
 * Frees up any resources allocated by the socket handle object.
 *
 * @warning This function won't close the socket, just frees up memory.
 *
 * @param sock Socket handle object to be free'd.
 */
void socket_free(sock_bundle_t *sock) {
	/* Do we even have anything to do? */
	if (sock == NULL)
		return;

	/* Free the address structure. */
	if (sock->addr != NULL) {
		free(sock->addr);
		sock->addr = NULL;
		sock->addr_len = 0;
	}

	/* Free ourselves. */
	free(sock);
}

/**
 * Sets up the address of a socket handle object using a network address string.
 *
 * @param sock Socket handle object to be setup.
 * @param addr Network address to bind/connect to as a string.
 * @param port Port to bind/connect to.
 *
 * @see socket_setup_inaddr
 */
void socket_setaddr(sock_bundle_t *sock, const char *addr, uint16_t port) {
	socket_setaddr_inaddr(sock,
		(addr == NULL) ? INADDR_ANY : socket_inet_addr(addr), port);
}

/**
 * Sets up the address of a socket handle object using an IPv4 address.
 *
 * @param sock   Socket handle object to be setup.
 * @param inaddr IPv4 address to connect/bind to already in the internal
 *               structure's format.
 * @param port   Port to bind/connect to.
 */
void socket_setaddr_inaddr(sock_bundle_t *sock, in_addr_t inaddr,
						   uint16_t port) {
	struct sockaddr_in *addr;

	/* Allocate the address structure. */
	sock->addr_len = sizeof(struct sockaddr_in);
	sock->addr = (struct sockaddr *)malloc(sock->addr_len);
	memset(sock->addr, 0, sock->addr_len);

	/* Setup the structure. */
	addr = (struct sockaddr_in *)sock->addr;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inaddr;
}

/**
 * Sets up a TCP socket.
 *
 * @param sock   Socket handle object.
 * @param server Should we start listening on the socket?
 *
 * @return SOCK_OK if the initialization was successful.
 *         SOCK_ERR_ESOCKET if the socket function failed.
 *         SOCK_ERR_ESETSOCKOPT if the setsockopt function failed.
 *         SOCK_ERR_EBIND if the bind function failed.
 *         TCP_ERR_ELISTEN if the listen function failed.
 */
sock_err_t socket_setup_tcp(sock_bundle_t *sock, bool server) {
#ifdef _WIN32
	char reuse;
#else
	int reuse;
#endif /* _WIN32 */

	/* Create a new TCP socket file descriptor. */
	sock->fd = socket(PF_INET, SOCK_STREAM, 0);
	if (sock->fd == INVALID_SOCKET) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to create TCP socket"),
					  sockerrno);
		return SOCK_ERR_ESOCKET;
	}

	/* Should we start listening on this socket or not? */
	if (!server)
		return SOCK_OK;

	/* Ensure we can reuse the address and port in case of panic. */
	reuse = 1;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno(LOG_WARNING, EMSG("Failed to set socket address reuse"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEPORT, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno(LOG_WARNING, EMSG("Failed to set socket port reuse"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Bind ourselves to the TCP address. */
	if (bind(sock->fd, sock->addr, sock->addr_len) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to bind ourselves to the socket"),
					  sockerrno);
		return SOCK_ERR_EBIND;
	}

	/* Start listening on our socket. */
	if (listen(sock->fd, TCPSERVER_BACKLOG) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to listen to the socket"),
					  sockerrno);
		return SOCK_ERR_ELISTEN;
	}

	return SOCK_OK;
}

/**
 * Sets up a UDP socket.
 *
 * @param sock       Socket handle object.
 * @param server     Should we start listening on the socket?
 * @param timeout_ms Timeout of the socket in milliseconds or 0 if we shouldn't
 *                   have a timeout.
 *
 * @return SOCK_OK if the operation was successful.
 *         SOCK_ERR_ESOCKET if the socket function failed.
 *         SOCK_ERR_ESETSOCKOPT if the setsockopt function failed.
 *         SOCK_ERR_EBIND if the bind function failed.
 */
sock_err_t socket_setup_udp(sock_bundle_t *sock, bool server,
							uint32_t timeout_ms) {
	unsigned char loop;
#ifdef _WIN32
	char reuse;
	ULONG perm;
#else
	int reuse;
	int perm;
	struct timeval tv;
#endif /* _WIN32 */

	/* Create a new UDP socket file descriptor. */
	sock->fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock->fd == INVALID_SOCKET) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to create UDP socket"),
					  sockerrno);
		return SOCK_ERR_ESOCKET;
	}

	/* Set a timeout for our packets. */
	if (timeout_ms > 0) {
#ifdef _WIN32
		if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO,
				(const char *)&timeout_ms, sizeof(DWORD)) == SOCKET_ERROR) {
			log_sockerrno(LOG_ERROR, EMSG("Failed to set socket timeout"),
						  sockerrno);
			return SOCK_ERR_ESETSOCKOPT;
		}
#else
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
					   sizeof(struct timeval)) == SOCKET_ERROR) {
			log_sockerrno(LOG_ERROR, EMSG("Failed to set socket timeout"),
						  sockerrno);
			return SOCK_ERR_ESETSOCKOPT;
		}
#endif
	}

	/* Should we start listening on this socket or not? */
	if (!server)
		return SOCK_OK;

	/* Ensure we can reuse the address and port in case of panic. */
	reuse = 1;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno(LOG_WARNING, EMSG("Failed to set socket address reuse"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEPORT, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno(LOG_WARNING, EMSG("Failed to set socket port reuse"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Ensure we can do UDP broadcasts. */
	perm = 1;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_BROADCAST, &perm,
				   sizeof(perm)) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to enable broadcast for socket"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}

	/* Ensure that we don't receive broadcasts from ourselves. */
	loop = 0;
#ifdef IP_MULTICAST_LOOP
	if (setsockopt(sock->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
				   sizeof(loop)) == SOCKET_ERROR) {
		log_sockerrno(LOG_WARNING,
					  EMSG("Failed to disable socket multicast loop"),
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Bind ourselves to the UDP address. */
	if (bind(sock->fd, sock->addr, sock->addr_len) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to bind ourselves to the socket"),
					  sockerrno);
		return SOCK_ERR_EBIND;
	}

	return SOCK_OK;
}

/**
 * Accepts an incoming connection.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param socket Server socket handle object.
 *
 * @return Newly allocated server client connection socket handle object or NULL
 *         in case of an error.
 */
sock_bundle_t *socket_accept(sock_bundle_t *server) {
	sock_bundle_t *client;

	/* Check if we even have a socket to accept things from. */
	if (server->fd == INVALID_SOCKET)
		return NULL;

	/* Allocate our client handle object. */
	client = socket_new();
	if (client == NULL)
		return NULL;

	/* Accept the new connection. */
	client->addr_len = sizeof(struct sockaddr_storage);
	client->fd = accept(server->fd, client->addr, &client->addr_len);

	/* Handle errors. */
	if (client->fd == INVALID_SOCKET) {
		bool disp_err;

		/* Make sure we can handle a shutdown cleanly. */
		if (server->fd == INVALID_SOCKET) {
			socket_free(client);
			return NULL;
		}

		/* Check if the error is worth displaying. */
#ifdef _WIN32
		disp_err = (sockerrno != WSAEBADF) && (sockerrno != WSAECONNABORTED);
#else
		disp_err = (errno != EBADF) && (errno != ECONNABORTED);
#endif /* _WIN32 */
		if (disp_err) {
			log_sockerrno(LOG_ERROR,
						  EMSG("Failed to accept incoming connection"),
						  sockerrno);
		}
	}

	return client;
}

/**
 * Creates a connection with another computer.
 *
 * @param sock Socket handle object.
 *
 * @return SOCK_OK if the operation was successful.
 *         TCP_ERR_ECONNECT if the connect function failed.
 */
sock_err_t socket_connect(sock_bundle_t *sock) {
	/* Connect ourselves to the address. */
	if (connect(sock->fd, sock->addr, sock->addr_len) == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to connect to socket"),
					  sockerrno);
		return SOCK_ERR_ECONNECT;
	}

	return SOCK_OK;
}

/**
 * Sends some data to a TCP socket.
 *
 * @param sock     TCP socket handle object.
 * @param buf      Data to be sent.
 * @param len      Length of the data to be sent.
 * @param sent_len Pointer to store the number of bytes actually sent. Ignored
 *                 if NULL is passed.
 *
 * @return SOCK_OK if the operation was successful.
 *         SOCK_ERR_ESEND if the send function failed.
 *
 * @see send
 */
sock_err_t socket_send(const sock_bundle_t *sock, const void *buf, size_t len,
					   size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = send(sock->fd, buf, len, 0);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to send data over TCP"),
					  sockerrno);
		return SOCK_ERR_ESEND;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return SOCK_OK;
}

/**
 * Sends some data to a UDP socket.
 *
 * @param sock      UDP socket handle object.
 * @param buf       Data to be sent.
 * @param len       Length of the data to be sent.
 * @param sock_addr IPv4 or IPv6 address structure.
 * @param sock_len  Length of the socket address structure in bytes.
 * @param sent_len  Pointer to store the number of bytes actually sent. Ignored
 *                  if NULL is passed.
 *
 * @return SOCK_OK if the operation was successful.
 *         SOCK_ERR_ESEND if the send function failed.
 *
 * @see sendto
 */
sock_err_t socket_sendto(const sock_bundle_t *sock, const void *buf, size_t len,
						 const struct sockaddr *sock_addr, socklen_t sock_len,
						 size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = sendto(sock->fd, buf, len, 0, sock_addr, sock_len);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to send data over UDP"),
					  sockerrno);
		return SOCK_ERR_ESEND;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return SOCK_OK;
}

/**
 * Receive some data from a TCP socket.
 *
 * @param sock     TCP socket handle object.
 * @param buf      Buffer to store the received data.
 * @param buf_len  Length of the buffer to store the data.
 * @param recv_len Pointer to store the number of bytes actually received. Will
 *                 be ignored if NULL is passed.
 * @param peek     Should we just peek at the data to be received?
 *
 * @return SOCK_OK if we received some data.
 *         SOCK_EVT_CONN_CLOSED if the connection was closed by the client.
 *         SOCK_ERR_ERECV if the recv function failed.
 *
 * @see recv
 */
sock_err_t socket_recv(const sock_bundle_t *sock, void *buf, size_t buf_len,
					   size_t *recv_len, bool peek) {
	size_t bytes_recv;
	ssize_t len;

	/* Check if we have a valid file descriptor. */
	if (sock->fd == INVALID_SOCKET)
		return SOCK_EVT_CONN_CLOSED;

	if (peek) {
		/* Peek at the data in the queue. */
		len = recv(sock->fd, buf, buf_len, MSG_PEEK);
		if (len == SOCKET_ERROR) {
			/* Check if it's just the connection being abruptly shut down. */
#ifdef _WIN32
			if (sockerrno == WSAEBADF)
#else
			if (errno == EBADF)
#endif /* _WIN32 */
				return SOCK_EVT_CONN_SHUTDOWN;

			log_sockerrno(LOG_ERROR, EMSG("Failed to peek at incoming TCP data"),
						  sockerrno);
			return SOCK_ERR_ERECV;
		}

		bytes_recv = len;
	} else {
		uint8_t *tmp;

		tmp = (uint8_t *)buf;
		bytes_recv = 0;
		while (bytes_recv < buf_len) {
			/* Try to read all the information from a socket. */
			len = recv(sock->fd, tmp, buf_len - bytes_recv, 0);
			if (len == SOCKET_ERROR) {
				/* Check if it's just the connection being abruptly shut down. */
#ifdef _WIN32
				if (sockerrno == WSAEBADF)
#else
				if (errno == EBADF)
#endif /* _WIN32 */
					return SOCK_EVT_CONN_SHUTDOWN;

				log_sockerrno(LOG_ERROR,
							  EMSG("Failed to receive incoming TCP data"),
							  sockerrno);
				return SOCK_ERR_ERECV;
			}

			bytes_recv += len;
			tmp += len;
		}
	}

	/* Return the number of bytes sent. */
	if (recv_len != NULL)
		*recv_len = bytes_recv;

	/* Check if the connection was closed gracefully by the client. */
	if (bytes_recv == 0)
		return SOCK_EVT_CONN_CLOSED;

	return SOCK_OK;
}

/**
 * Receive some data from a UDP socket.
 *
 * @param sock      UDP socket handle object.
 * @param buf       Buffer to store the received data.
 * @param buf_len   Length of the buffer to store the data.
 * @param sock_addr Pointer to an IPv4 or IPv6 address structure to store the
 *                  client's address information.
 * @param sock_len  Pointer to length of the socket address structure in bytes.
 * @param recv_len  Pointer to store the number of bytes actually received. Will
 *                  be ignored if NULL is passed.
 * @param peek      Should we just peek at the data to be received?
 *
 * @return SOCK_OK if we received some data.
 *         SOCK_EVT_CONN_CLOSED if the connection was closed by the client.
 *         SOCK_ERR_ERECV if the recv function failed.
 *
 * @see recvfrom
 */
sock_err_t socket_recvfrom(const sock_bundle_t *sock, void *buf, size_t buf_len,
						   struct sockaddr *sock_addr, socklen_t *sock_len,
						   size_t *recv_len, bool peek) {
	ssize_t bytes_recv;

	/* Check if we have a valid file descriptor. */
	if (sock->fd == INVALID_SOCKET)
		return SOCK_EVT_CONN_CLOSED;

	/* Try to read some information from a socket. */
	bytes_recv = recvfrom(sock->fd, buf, buf_len, (peek) ? MSG_PEEK : 0,
						  sock_addr, sock_len);
#ifdef _WIN32
	if (bytes_recv == SOCKET_ERROR) {
		/* Check if it was just a message bigger than the peek'd length. */
		if (peek && (sockerrno == WSAEMSGSIZE)) {
			bytes_recv = buf_len;
			goto recvnorm;
		}

		/* Check if the error was expected. */
		if (sockerrno == WSAETIMEDOUT) {
			/* Timeout occurred. */
			return SOCK_EVT_TIMEOUT;
		} else if (sockerrno == WSAEINTR) {
			/* Connection being abruptly shut down. */
			return SOCK_EVT_CONN_SHUTDOWN;
		}

		/* Looks like it was a proper error. */
		log_sockerrno(LOG_ERROR, EMSG("Failed to peek at incoming UDP data"),
					  sockerrno);
		return SOCK_ERR_ERECV;
	}
recvnorm:
#else
	if (bytes_recv == SOCKET_ERROR) {
		/* Check if the error was expected. */
		if (errno == EAGAIN) {
			/* Timeout occurred. */
			return SOCK_EVT_TIMEOUT;
		} else if (errno == EBADF) {
			/* Connection being abruptly shut down. */
			return SOCK_EVT_CONN_SHUTDOWN;
		}

		/* Looks like it was a proper error. */
		log_sockerrno(LOG_ERROR, EMSG("Failed to receive incoming UDP data"),
					  sockerrno);
		return SOCK_ERR_ERECV;
	}
#endif /* _WIN32 */

	/* Return the number of bytes sent. */
	if (recv_len != NULL)
		*recv_len = bytes_recv;

	/* Check if it's just the connection being abruptly shut down. */
	if (bytes_recv == 0)
		return SOCK_EVT_CONN_CLOSED;

	return SOCK_OK;
}

/**
 * Closes a socket handle.
 *
 * @param sock Server handle object.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 */
sock_err_t socket_close(sock_bundle_t *sock) {
	int ret;
	sock_err_t err = SOCK_OK;

	/* Check if we are even needed. */
	if ((sock == NULL) || (sock->fd == INVALID_SOCKET))
		return SOCK_OK;

	/* Close the socket file descriptor. */
#ifdef _WIN32
	ret = closesocket(sock->fd);
#else
	ret = close(sock->fd);
#endif /* _WIN32 */

	/* Check for errors. */
	if (ret == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to close the socket"), sockerrno);
		err = SOCK_ERR_ECLOSE;

		goto endclose;
	}

endclose:
	/* Invalidate socket and return. */
	sock->fd = INVALID_SOCKET;
	return err;
}

/**
 * Shuts down a socket handle.
 *
 * @param sock Server handle object.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ESHUTDOWN if the socket failed to shutdown properly.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 */
sock_err_t socket_shutdown(sock_bundle_t *sock) {
	int ret;
	sock_err_t err = SOCK_OK;

	/* Check if we are even needed. */
	if ((sock == NULL) || (sock->fd == INVALID_SOCKET))
		return SOCK_OK;

	/* Shutdown the socket file descriptor. */
#ifdef _WIN32
	ret = shutdown(sockfd, SD_BOTH);
	if ((ret == SOCKET_ERROR) && (sockerrno != WSAENOTCONN)) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to shutdown socket"), sockerrno);
		err = SOCK_ERR_ESHUTDOWN;
		goto endshutdown;
	}

	ret = closesocket(sockfd);
	if (ret == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to close socket after shutdown"),
					  sockerrno);
		err = SOCK_ERR_ECLOSE;
		goto endshutdown;
	}
#else
	ret = shutdown(sock->fd, SHUT_RDWR);
	if ((ret == SOCKET_ERROR) && (errno != ENOTCONN) && (errno != EINVAL)) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to shutdown socket"), sockerrno);
		err = SOCK_ERR_ESHUTDOWN;
		goto endshutdown;
	}

	ret = close(sock->fd);
	if (ret == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to close socket after shutdown"),
					  sockerrno);
		err = SOCK_ERR_ECLOSE;
		goto endshutdown;
	}
#endif /* _WIN32 */

endshutdown:
	/* Invalidate socket and return. */
	sock->fd = INVALID_SOCKET;
	return err;
}

/**
 * Converts an IPv4 or IPv6 address from binary to a presentation format string
 * representation.
 * @warning This function will allocate memory that must be free'd by you.
 *
 * @param buf       Pointer to a string that will be populated with the
 *                  presentation format IP address.
 * @param sock_addr Generic IPv4 or IPv6 structure containing the address to be
 *                  converted.
 *
 * @return TRUE if the conversion was successful.
 *         FALSE if an error occurred and we couldn't convert the IP address.
 */
bool socket_itos(char **buf, const struct sockaddr *sock_addr) {
#ifdef _WIN32
	TCHAR tmp[INET6_ADDRSTRLEN];
	DWORD dwLen;

	dwLen = INET6_ADDRSTRLEN;
#else
	char tmp[INET6_ADDRSTRLEN];
#endif /* _WIN32 */

	/* Determine which type of IP address we are dealing with. */
	switch (sock_addr->sa_family) {
		case AF_INET:
#ifdef _WIN32
			WSAAddressToString(sock_addr, sizeof(struct sockaddr_in), NULL,
							   tmp, &dwLen);
#else
			inet_ntop(AF_INET,
					  &(((const struct sockaddr_in *)sock_addr)->sin_addr),
					  tmp, INET_ADDRSTRLEN);
#endif /* _WIN32 */
			break;
		case AF_INET6:
#ifdef _WIN32
			WSAAddressToString(sock_addr, sizeof(struct sockaddr_in6), NULL,
							   tmp, &dwLen);
#else
			inet_ntop(AF_INET6,
					  &(((const struct sockaddr_in6 *)sock_addr)->sin6_addr),
					  tmp, INET6_ADDRSTRLEN);
#endif /* _WIN32 */
			break;
		default:
			*buf = NULL;
			return false;
	}

#ifdef _WIN32
	/* Remove the port number from the string. */
	for (dwLen = 0; tmp[dwLen] != '\0'; dwLen++) {
		if (tmp[dwLen] == ':') {
			tmp[dwLen] = '\0';
			break;
		}
	}

	/* Convert our string to UTF-8 assigning it to the return value. */
	*buf = utf16_wcstombs(tmp);
#else
	/* Allocate space for our return string. */
	*buf = (char *)malloc((strlen(tmp) + 1) * sizeof(char));
	if (*buf == NULL) {
		log_errno(LOG_FATAL, EMSG("Failed to allocate memory for string "
								  "representation of IP address"));
		return false;
	}

	/* Copy our IP address over and return. */
	strcpy(*buf, tmp);
#endif /* _WIN32 */

	return true;
}

/**
 * Converts an IPv4 address represented as a string to its binary form.
 *
 * @param ipaddr IPv4 address in its string representation.
 *
 * @return Binary representation of the IPv4 address or INADDR_NONE (-1) if an
 *         error occurred.
 */
in_addr_t socket_inet_addr(const char *ipaddr) {
#ifdef _WIN32
	wchar_t *ipaw;
	struct sockaddr_in addr;
	int len;

	/* Convert our IP address string to UTF-16. */
	ipaw = utf16_mbstowcs(ipaddr);
	if (ipaw == NULL)
		return INADDR_NONE;

	/* Get the address from the string. */
	len = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	WSAStringToAddress(ipaw, AF_INET, NULL, (struct sockaddr *)&addr, &len);

	/* Free our temporary buffer. */
	free(ipaw);

	return addr.sin_addr.S_un.S_addr;
#else
	return inet_addr(ipaddr);
#endif /* _WIN32 */
}

#ifndef SINGLE_IFACE_MODE
/**
 * Allocates a brand new network interface information object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated empty network interface information object or NULL
 *         if an error occurred.
 *
 * @see socket_iface_info_list
 * @see socket_iface_info_free
 */
iface_info_t *socket_iface_info_new(void) {
	iface_info_t *iface;

	/* Allocate the object. */
	iface = (iface_info_t *)malloc(sizeof(iface_info_t));
	if (iface == NULL) {
		log_errno(LOG_FATAL, EMSG("Failed to allocate memory for network "
								  "interface information"));
		return NULL;
	}

	/* Ensure the emptiness of our object. */
	iface->name = NULL;
	iface->ifaddr = NULL;
	iface->brdaddr = NULL;

	return iface;
}

/**
 * Gets a list of all of the network interfaces in the system that are currently
 * capable of broadcasting an UDP packet.
 *
 * @param if_list Pointer to an uninitialized network interface information list
 *                object. (Will be allocated and populated by this function)
 *
 * @return SOCK_OK if the operation was successful.
 *         IFACE_ERR_GETIFADDR if the getifaddr function failed.
 *
 * @see socket_iface_info_list_free
 */
sock_err_t socket_iface_info_list(iface_info_list_t **if_list) {
	iface_info_list_t *list;
	int ret;
#ifdef _WIN32
	SOCKET sock;
	INTERFACE_INFO aifi[20];
	DWORD dwSize;
	ULONG ulInterfaces;
	UINT8 i;
#else
	struct ifaddrs *ifa_list;
	struct ifaddrs *ifa;
#endif /* _WIN32 */

	/* Allocate our list object. */
	list = (iface_info_list_t *)malloc(sizeof(iface_info_list_t));
	if (list == NULL) {
		log_errno(LOG_FATAL, EMSG("Failed to allocate memory for network "
								  "interface information list"));
		*if_list = NULL;

		return SOCK_ERR_UNKNOWN;
	}

	/* Initialize the list. */
	list->ifaces = NULL;
	list->count = 0;

#ifdef _WIN32
	/* Create a dummy socket for querying. */
	sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (sock == INVALID_SOCKET) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to create dummy socket for "
									  "network interface querying"), sockerrno);
		socket_iface_info_list_free(list);
		*if_list = NULL;

		return SOCK_ERR_ESOCKET;
	}

	/* Get network interface list. */
	ret = WSAIoctl(sock, SIO_GET_INTERFACE_LIST, 0, 0, &aifi, sizeof(aifi),
				   &dwSize, 0, 0);
	if (ret == SOCKET_ERROR) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to ioctl dummy query socket"),
					  sockerrno);
		socket_iface_info_list_free(list);
		*if_list = NULL;

		return SOCK_ERR_EIOCTL;
	}

	/* Get the number of interfaces returned and go through them. */
	ulInterfaces = dwSize / sizeof(INTERFACE_INFO);
	for (i = 0; i < ulInterfaces; i++) {
		struct sockaddr *sa;
		ULONG ulFlags;

		/* Check if the interface is active and usable for discovery. */
		sa = (struct sockaddr *)&aifi[i].iiAddress;
		ulFlags = aifi[i].iiFlags;
		if ((ulFlags & IFF_UP) && (ulFlags & IFF_BROADCAST) &&
				!(ulFlags & IFF_LOOPBACK)) {
			INTERFACE_INFO ii;
			iface_info_t *iface;

			/* Get the current interface info object. */
			ii = aifi[i];

			/* Build up an information object. */
			iface = socket_iface_info_new();
			iface->name = NULL;
			if (sa->sa_family == AF_INET) {
				/* Duplicate the IPv4 sockaddr objects. */
				iface->ifaddr = (struct sockaddr *)memdup(&ii.iiAddress,
					sizeof(struct sockaddr_in));
				iface->brdaddr = (struct sockaddr *)memdup(&ii.iiAddress,
					sizeof(struct sockaddr_in));

				/* Calculate the broadcast address since Windows can't. */
				((struct sockaddr_in *)(iface->brdaddr))->sin_addr.s_addr |=
					~((struct sockaddr_in *)(&ii.iiNetmask))->sin_addr.s_addr;
			} else {
				/* Duplicate the IPv6 sockaddr objects. */
				iface->ifaddr = (struct sockaddr *)memdup(&ii.iiAddress,
					sizeof(struct sockaddr_in6));
				iface->brdaddr = (struct sockaddr *)memdup(
					&ii.iiBroadcastAddress, sizeof(struct sockaddr_in6));
			}

			/* Append the information object to the list. */
			if (socket_iface_info_list_push(list, iface) != SOCK_OK) {
				log_errno(LOG_ERROR, EMSG("Failed to push network interface "
										  "information to the list"));
				socket_iface_info_list_free(list);
				*if_list = NULL;

				return TCP_ERR_UNKNOWN;
			}
		}
	}
#else
	/* Get the linked list of network interfaces. */
	ret = getifaddrs(&ifa_list);
	if (ret == -1) {
		log_sockerrno(LOG_ERROR, EMSG("Failed to get the network interface "
									  "address"), sockerrno);
		socket_iface_info_list_free(list);
		*if_list = NULL;

		return IFACE_ERR_GETIFADDR;
	}

	/* Go through the list of network interfaces. */
	for (ifa = ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
		iface_info_t *iface;

		/* Discard the ones that don't have an IP address. */
		if (ifa->ifa_addr == NULL)
			continue;

		/* TODO: Remove this when support for IPv6 is added. */
		/* TODO: Look into supporting VPN interfaces. */
		/* Discard the ones that aren't IPv4. */
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		/* Discard the ones that can't be broadcast. */
		if ((ifa->ifa_flags & IFF_BROADCAST) == 0)
			continue;

		/* Build up an information object. */
		iface = socket_iface_info_new();
		iface->name = strdup(ifa->ifa_name);
		if (ifa->ifa_addr->sa_family == AF_INET) {
			iface->ifaddr = (struct sockaddr *)memdup(ifa->ifa_addr,
				sizeof(struct sockaddr_in));
			iface->brdaddr = (struct sockaddr *)memdup(ifa->ifa_broadaddr,
				sizeof(struct sockaddr_in));
		} else {
			iface->ifaddr = (struct sockaddr *)memdup(ifa->ifa_addr,
				sizeof(struct sockaddr_in6));
			iface->brdaddr = (struct sockaddr *)memdup(ifa->ifa_broadaddr,
				sizeof(struct sockaddr_in6));
		}

		/* Append the information object to the list. */
		if (socket_iface_info_list_push(list, iface) != SOCK_OK) {
			log_errno(LOG_ERROR, EMSG("Failed to push network interface "
									  "information to the list"));
			socket_iface_info_list_free(list);
			*if_list = NULL;

			return SOCK_ERR_UNKNOWN;
		}
	}

	/* Free our network interface linked list. */
	freeifaddrs(ifa_list);
#endif /* _WIN32 */

	/* Return our list. */
	*if_list = list;

	return SOCK_OK;
}

/**
 * Appends an network interface information object into the network interfaces
 * list.
 *
 * @param if_list Network interface information object list.
 * @param iface   Network interface information object to be appended.
 *
 * @return SOCK_OK if the operation was successful.
 */
sock_err_t socket_iface_info_list_push(iface_info_list_t *if_list,
									  iface_info_t *iface) {
	/* Resize the list. */
	if_list->count++;
	if_list->ifaces = (iface_info_t **)realloc(if_list->ifaces,
		sizeof(iface_info_t *) * if_list->count);
	if (if_list->ifaces == NULL) {
		log_errno(LOG_FATAL, EMSG("Failed to reallocate memory for the network "
								  "interface list"));
		socket_iface_info_free(iface);

		return SOCK_ERR_UNKNOWN;
	}

	/* Append the network interface information object to the list. */
	if_list->ifaces[if_list->count - 1] = iface;

	return SOCK_OK;
}

/**
 * Frees up any resources allocated by a network interface information object
 * list.
 *
 * @param if_list Network interface information object list to be free'd.
 */
void socket_iface_info_list_free(iface_info_list_t *if_list) {
	uint8_t i;

	/* Do we even have anything to do? */
	if (if_list == NULL)
		return;

	/* Free the interface list. */
	if (if_list->ifaces) {
		/* Free each interface of the list. */
		for (i = 0; i < if_list->count; i++)
			socket_iface_info_free(if_list->ifaces[i]);

		/* Free the list itself. */
		free(if_list->ifaces);
		if_list->ifaces = NULL;
	}

	/* Reset the count and free ourselves. */
	if_list->count = 0;
	free(if_list);
}

/**
 * Frees up any resources allocated by a network interface information object.
 *
 * @param iface Network interface information object to be free'd.
 */
void socket_iface_info_free(iface_info_t *iface) {
	/* Do we even have anything to do? */
	if (iface == NULL)
		return;

	/* Free its parameters. */
	if (iface->name)
		free(iface->name);
	if (iface->ifaddr)
		free(iface->ifaddr);
	if (iface->brdaddr)
		free(iface->brdaddr);

	/* Free ourselves. */
	free(iface);
}
#endif /* !SINGLE_IFACE_MODE */
