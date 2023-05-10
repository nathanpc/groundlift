/**
 * sockets.c
 * Socket server/client that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <utils/utf16.h>
#else
#include <sys/errno.h>
#include <sys/time.h>
#include <unistd.h>
#endif /* _WIN32 */

#include "defaults.h"
#include "error.h"

/* Cross-platform socket function return error code. */
#ifndef SOCKET_ERROR
	#define SOCKET_ERROR (-1)
#endif /* !SOCKET_ERROR */

/* Cross-platform representation of an invalid socket file descriptor. */
#ifndef INVALID_SOCKET
	#ifdef _WIN32
		#define INVALID_SOCKET (SOCKET)(~0)
	#else
		#define INVALID_SOCKET (-1)
	#endif /* _WIN32 */
#endif /* !INVALID_SOCKET */

/* Cross-platform shim for socket error codes. */
#ifdef _WIN32
#define sockerrno WSAGetLastError()
#else
#define sockerrno errno
#endif /* _WIN32 */

/**
 * Creates a brand new server handle object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param addr Address to bind ourselves to or NULL to use INADDR_ANY.
 * @param port TCP port to listen on for file transfers.
 *
 * @return Newly allocated server handle object or NULL if we couldn't allocate
 *         the object.
 *
 * @see sockets_server_free
 */
server_t *sockets_server_new(const char *addr, uint16_t port) {
	server_t *server;

	/* Allocate some memory for our handle object. */
	server = (server_t *)malloc(sizeof(server_t));
	if (server == NULL)
		return NULL;

	/* Ensure we have a known invalid state for our socket file descriptors. */
	server->tcp.sockfd = INVALID_SOCKET;
	server->udp.sockfd = INVALID_SOCKET;

	/* Setup the socket address structure for binding. */
	server->tcp.addr_in_size = socket_setup(&server->tcp.addr_in, addr, port);

	return server;
}

/**
 * Creates a brand new client handle object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param addr IP address to connect to.
 * @param port Port to connect on.
 *
 * @return Newly allocated client handle object or NULL if we couldn't allocate
 *         the object.
 *
 * @see tcp_client_free
 */
tcp_client_t *tcp_client_new(const char *addr, uint16_t port) {
	tcp_client_t *client;

	/* Allocate some memory for our handle object. */
	client = (tcp_client_t *)malloc(sizeof(tcp_client_t));
	if (client == NULL)
		return NULL;

	/* Ensure we have a known invalid state for our socket file descriptor. */
	client->sockfd = INVALID_SOCKET;
	client->packet_len = OBEX_MAX_PACKET_SIZE;

	/* Setup the socket address structure for connecting. */
	client->addr_in_size = socket_setup(&client->addr_in, addr, port);

	return client;
}

/**
 * Frees up any resources allocated by the server.
 *
 * @param server Server handle object to be free'd.
 *
 * @see sockets_server_stop
 */
void sockets_server_free(server_t *server) {
	/* Do we even need to do something? */
	if (server == NULL)
		return;

	/* Free the object and NULL it out. */
	free(server);
	server = NULL;
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
 * Frees up any resources allocated by the client connection.
 *
 * @param server Client handle object to be free'd.
 *
 * @see tcp_client_close
 */
void tcp_client_free(tcp_client_t *client) {
	/* Do we even need to do something? */
	if (client == NULL)
		return;

	/* Free the object and NULL it out. */
	free(client);
	client = NULL;
}

/**
 * Starts up the server.
 *
 * @param server Server handle object.
 *
 * @return SOCK_OK if the initialization was successful.
 *         SOCK_ERR_ESOCKET if the socket function failed.
 *         SOCK_ERR_ESETSOCKOPT if the setsockopt function failed.
 *         SOCK_ERR_EBIND if the bind function failed.
 *         TCP_ERR_ELISTEN if the listen function failed.
 *
 * @see sockets_server_stop
 */
tcp_err_t sockets_server_start(server_t *server) {
#ifdef _WIN32
	char reuse;
#else
	int reuse;
#endif /* _WIN32 */

	/* Create a new TCP socket file descriptor. */
	server->tcp.sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (server->tcp.sockfd == INVALID_SOCKET) {
		log_sockerrno("sockets_server_start@socket", sockerrno);
		return SOCK_ERR_ESOCKET;
	}

	/* Ensure we can reuse the address and port in case of panic. */
	reuse = 1;
	if (setsockopt(server->tcp.sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno("sockets_server_start@setsockopt(SO_REUSEADDR)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(server->tcp.sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno("sockets_server_start@setsockopt(SO_REUSEPORT)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Bind ourselves to the TCP address. */
	if (bind(server->tcp.sockfd, (struct sockaddr *)&server->tcp.addr_in,
			 server->tcp.addr_in_size) == SOCKET_ERROR) {
		log_sockerrno("sockets_server_start@bind", sockerrno);
		return SOCK_ERR_EBIND;
	}

	/* Start listening on our socket. */
	if (listen(server->tcp.sockfd, TCPSERVER_BACKLOG) == SOCKET_ERROR) {
		log_sockerrno("sockets_server_start@listen", sockerrno);
		return TCP_ERR_ELISTEN;
	}

	return SOCK_OK;
}

/**
 * Stops the server but doesn't free the resources allocated.
 *
 * @param server Server handle object.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 *
 * @see sockets_server_shutdown
 * @see sockets_server_free
 */
tcp_err_t sockets_server_stop(server_t *server) {
	tcp_err_t err;

	/* Ensure we actually have something do to. */
	if (server == NULL)
		return SOCK_OK;

	/* Close the socket file descriptor and set it to a known invalid state. */
	err = socket_close(server->udp.sockfd);
	server->udp.sockfd = INVALID_SOCKET;
	if (err > SOCK_OK)
		fprintf(stderr, "sockets_server_stop: Failed to close UDP socket.\n");
	err = socket_close(server->tcp.sockfd);
	server->tcp.sockfd = INVALID_SOCKET;

	return err;
}

/**
 * Shutdown the server but doesn't free the resources allocated. This is similar
 * to a close, except that it'll always unblock accept and recv.
 *
 * @param server Server handle object.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 *         TCP_ERR_ESHUTDOWN if the socket failed to shutdown properly.
 *
 * @see sockets_server_stop
 * @see sockets_server_free
 */
tcp_err_t sockets_server_shutdown(server_t *server) {
	tcp_err_t err;

	/* Ensure we actually have something do to. */
	if (server == NULL)
		return SOCK_OK;

	/* Shutdown the socket and set it to a known invalid state. */
	err = socket_shutdown(server->udp.sockfd);
	server->udp.sockfd = INVALID_SOCKET;
	if (err > SOCK_OK)
		fprintf(stderr, "sockets_server_shutdown: UDP socket close failed.\n");
	err = socket_shutdown(server->tcp.sockfd);
	server->tcp.sockfd = INVALID_SOCKET;

	return err;
}

/**
 * Initializes a discovery service UDP socket bundle structure and sets up the
 * socket.
 *
 * @param sock       Socket bundle structure.
 * @param server     Is the service being
 * @param in_addr    IP address to connect/bind to already in the internal
 *                   structure's format.
 * @param port       Port to connect/bind on.
 * @param timeout_ms Timeout of the socket in milliseconds or 0 if we shouldn't
 *                   have a timeout.
 *
 * @return SOCK_OK if the operation was successful.
 *         SOCK_ERR_ESOCKET if the socket function failed.
 *         SOCK_ERR_ESETSOCKOPT if the setsockopt function failed.
 *         SOCK_ERR_EBIND if the bind function failed.
 */
tcp_err_t udp_discovery_init(sock_bundle_t *sock, bool server,
							 in_addr_t in_addr, uint16_t port,
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

	/* Setup the socket bundle. */
	sock->sockfd = INVALID_SOCKET;
	sock->addr_in_size = socket_setup_inaddr(&sock->addr_in, in_addr, port);

	/* Create a new UDP socket file descriptor. */
	sock->sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock->sockfd == INVALID_SOCKET) {
		log_sockerrno("udp_discovery_init@socket", sockerrno);
		return SOCK_ERR_ESOCKET;
	}

	/* Ensure we can reuse the address and port in case of panic. */
	reuse = 1;
	if (setsockopt(sock->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno("udp_discovery_init@setsockopt(SO_REUSEADDR)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sock->sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse,
				   sizeof(reuse)) == SOCKET_ERROR) {
		log_sockerrno("udp_discovery_init@setsockopt(SO_REUSEPORT)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Ensure we can do UDP broadcasts. */
	perm = 1;
	if (setsockopt(sock->sockfd, SOL_SOCKET, SO_BROADCAST, &perm,
				   sizeof(perm)) == SOCKET_ERROR) {
		log_sockerrno("udp_discovery_init@setsockopt(SO_BROADCAST)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}

	/* Ensure that we don't receive broadcasts from ourselves. */
	loop = 0;
#ifdef IP_MULTICAST_LOOP
	if (setsockopt(sock->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
				   sizeof(loop)) == SOCKET_ERROR) {
		log_sockerrno("udp_discovery_init@setsockopt(IP_MULTICAST_LOOP)",
					  sockerrno);
		return SOCK_ERR_ESETSOCKOPT;
	}
#endif

	/* Set a timeout for our packets. */
	if (timeout_ms > 0) {
#ifdef _WIN32
		if (setsockopt(sock->sockfd, SOL_SOCKET, SO_RCVTIMEO,
				(const char *)&timeout_ms, sizeof(DWORD)) == SOCKET_ERROR) {
			log_sockerrno("udp_discovery_init@setsockopt(SO_RCVTIMEO)",
						  sockerrno);
			return SOCK_ERR_ESETSOCKOPT;
		}
#else
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		if (setsockopt(sock->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
					   sizeof(struct timeval)) == SOCKET_ERROR) {
			log_sockerrno("udp_discovery_init@setsockopt(SO_RCVTIMEO)",
						  sockerrno);
			return SOCK_ERR_ESETSOCKOPT;
		}
#endif
	}

	/* Bind ourselves to the UDP address. */
	if (server && (bind(sock->sockfd, (struct sockaddr *)&sock->addr_in,
						sock->addr_in_size) == SOCKET_ERROR)) {
		log_sockerrno("udp_discovery_init@bind", sockerrno);
		return SOCK_ERR_EBIND;
	}

	return SOCK_OK;
}

/**
 * Accepts an incoming connection.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param server Server handle object.
 *
 * @return Newly allocated server client connection handle object.
 *         NULL in case of an error.
 *
 * @see tcp_server_conn_free
 */
server_conn_t *tcp_server_conn_accept(const server_t *server) {
	server_conn_t *conn;

	/* Check if we even have a socket to accept things from. */
	if (server->tcp.sockfd == INVALID_SOCKET)
		return NULL;

	/* Allocate some memory for our handle object. */
	conn = (server_conn_t *)malloc(sizeof(server_conn_t));
	if (conn == NULL)
		return NULL;

	/* Accept the new connection. */
	conn->addr_size = sizeof(struct sockaddr_storage);
	conn->sockfd = accept(server->tcp.sockfd, (struct sockaddr *)&conn->addr,
						  &conn->addr_size);
	conn->packet_len = OBEX_MAX_PACKET_SIZE;

	/* Handle errors. */
	if (conn->sockfd == INVALID_SOCKET) {
		/* Make sure we can handle a shutdown cleanly. */
		if (server->tcp.sockfd == INVALID_SOCKET) {
			free(conn);
			return NULL;
		}

		/* Check if the error is worth displaying. */
#ifdef _WIN32
		if ((sockerrno != WSAEBADF) && (sockerrno != WSAECONNABORTED))
#else
		if ((errno != EBADF) && (errno != ECONNABORTED))
#endif /* _WIN32 */
			log_sockerrno("tcp_server_conn_accept@accept", sockerrno);
	}

	return conn;
}

/**
 * Connects to a client to a server.
 *
 * @param client Client handle object.
 *
 * @return SOCK_OK if the operation was successful.
 *         SOCK_ERR_ESOCKET if the socket function failed.
 *         TCP_ERR_ECONNECT if the connect function failed.
 */
tcp_err_t tcp_client_connect(tcp_client_t *client) {
	/* Create a new socket file descriptor. */
	client->sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (client->sockfd == INVALID_SOCKET) {
		log_sockerrno("tcp_client_connect@socket", sockerrno);
		return SOCK_ERR_ESOCKET;
	}

	/* Connect ourselves to the address. */
	if (connect(client->sockfd, (struct sockaddr *)&client->addr_in,
				client->addr_in_size) == SOCKET_ERROR) {
		log_sockerrno("tcp_client_connect@connect", sockerrno);
		return TCP_ERR_ECONNECT;
	}

	return SOCK_OK;
}

/**
 * Send some data to a client connected to our server.
 *
 * @param conn Server client connection handle object.
 * @param buf  Data to be sent.
 * @param len  Length of the data to be sent.
 *
 * @return SOCK_OK if the operation was successful.
 *         TCP_ERR_ESEND if the send function failed.
 *
 * @see tcp_socket_send
 */
tcp_err_t tcp_server_conn_send(const server_conn_t *conn, const void *buf,
							   size_t len) {
	return tcp_socket_send(conn->sockfd, buf, len, NULL);
}

/**
 * Send some data to the server we are connected to.
 *
 * @param client Client handle object.
 * @param buf    Data to be sent.
 * @param len    Length of the data to be sent.
 *
 * @return SOCK_OK if the operation was successful.
 *         TCP_ERR_ESEND if the send function failed.
 *
 * @see tcp_socket_send
 */
tcp_err_t tcp_client_send(const tcp_client_t *client, const void *buf,
						  size_t len) {
	return tcp_socket_send(client->sockfd, buf, len, NULL);
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
 * @return SOCK_OK if the operation was successful.
 *         TCP_ERR_ERECV if the recv function failed.
 *
 * @see tcp_socket_recv
 */
tcp_err_t tcp_server_conn_recv(const server_conn_t *conn, void *buf,
							   size_t buf_len, size_t *recv_len, bool peek) {
	return tcp_socket_recv(conn->sockfd, buf, buf_len, recv_len, peek);
}

/**
 * Receives some data from the server we are connected to.
 *
 * @param client   Client handle object.
 * @param buf      Buffer to store the received data.
 * @param buf_len  Length of the buffer to store the data.
 * @param recv_len Pointer to store the number of bytes actually received. Will
 *                 be ignored if NULL is passed.
 * @param peek     Should we just peek at the data to be received?
 *
 * @return SOCK_OK if the operation was successful.
 *         TCP_ERR_ERECV if the recv function failed.
 *
 * @see tcp_socket_recv
 */
tcp_err_t tcp_client_recv(const tcp_client_t *client, void *buf, size_t buf_len,
						  size_t *recv_len, bool peek) {
	return tcp_socket_recv(client->sockfd, buf, buf_len, recv_len, peek);
}

/**
 * Shuts down a server client's remote connection.
 *
 * @param conn Server client connection handle object.
 *
 * @return SOCK_OK if everything went fine.
 *         TCP_ERR_ECLOSE if the socket failed to shutdown properly.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 *
 * @see tcp_server_conn_free
 */
tcp_err_t tcp_server_conn_shutdown(server_conn_t *conn) {
	tcp_err_t err;

	/* Ensure we actually have something do to. */
	if ((conn == NULL) || (conn->sockfd == INVALID_SOCKET))
		return SOCK_OK;

	/* Close the socket file descriptor and set it to a known invalid state. */
	err = socket_shutdown(conn->sockfd);
	conn->sockfd = INVALID_SOCKET;

	return err;
}

/**
 * Closes a client connection to a server.
 *
 * @param client Client connection handle object.
 *
 * @return SOCK_OK if everything went fine.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 *
 * @see tcp_client_free
 */
tcp_err_t tcp_client_close(tcp_client_t *client) {
	tcp_err_t err;

	/* Ensure we actually have something do to. */
	if ((client == NULL) || (client->sockfd == INVALID_SOCKET))
		return SOCK_OK;

	/* Close the socket file descriptor and set it to a known invalid state. */
	err = socket_close(client->sockfd);
	client->sockfd = INVALID_SOCKET;

	return err;
}

/**
 * Closes a client connection to a server. This is similar to a close, except
 * that it'll always unblock connect and recv.
 *
 * @param client Client connection handle object.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         TCP_ERR_ECLOSE if the socket failed to close properly.
 *         TCP_ERR_ESHUTDOWN if the socket failed to shutdown properly.
 *
 * @see tcp_client_free
 * @see tcp_client_close
 */
tcp_err_t tcp_client_shutdown(tcp_client_t *client) {
	tcp_err_t err;

	/* Ensure we actually have something do to. */
	if ((client == NULL) || (client->sockfd == INVALID_SOCKET))
		return SOCK_OK;

	/* Shutdown the socket and set it to a known invalid state. */
	err = socket_shutdown(client->sockfd);
	client->sockfd = INVALID_SOCKET;

	return err;
}

/**
 * Sets up a socket address structure.
 *
 * @param addr   Pointer to a socket address structure to be populated.
 * @param inaddr IP address to connect/bind to already in the internal
 *               structure's format.
 * @param port   Port to bind/connect to.
 *
 * @return Size of the socket address structure.
 *
 * @see sockets_server_new
 */
socklen_t socket_setup_inaddr(struct sockaddr_in *addr, in_addr_t inaddr,
							  uint16_t port) {
	socklen_t addr_size;

	/* Setup the structure. */
	addr_size = sizeof(struct sockaddr_in);
	memset(addr, 0, addr_size);
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inaddr;

	return addr_size;
}

/**
 * Sets up a socket address structure using a string IP representation.
 *
 * @param addr   Pointer to a socket address structure to be populated.
 * @param ipaddr IP address to bind/connect to as a string.
 * @param port   Port to bind/connect to.
 *
 * @return Size of the socket address structure.
 *
 * @see sockets_server_new
 * @see tcp_client_new
 */
socklen_t socket_setup(struct sockaddr_in *addr, const char *ipaddr,
					   uint16_t port) {
	return socket_setup_inaddr(addr,
		(ipaddr == NULL) ? INADDR_ANY : inet_addr(ipaddr), port);
}

/**
 * Sends some data to a TCP socket file descriptor.
 *
 * @param sockfd   TCP Socket file descriptor.
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
tcp_err_t tcp_socket_send(int sockfd, const void *buf, size_t len,
						  size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = send(sockfd, buf, len, 0);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno("tcp_socket_send@send", sockerrno);
		return SOCK_ERR_ESEND;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return SOCK_OK;
}

/**
 * Sends some data to a UDP socket file descriptor.
 *
 * @param sockfd    UDP Socket file descriptor.
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
 * @see send
 */
tcp_err_t udp_socket_send(int sockfd, const void *buf, size_t len,
						  const struct sockaddr *sock_addr, socklen_t sock_len,
						  size_t *sent_len) {
	ssize_t bytes_sent;

	/* Try to send some information through a socket. */
	bytes_sent = sendto(sockfd, buf, len, 0, sock_addr, sock_len);
	if (bytes_sent == SOCKET_ERROR) {
		log_sockerrno("udp_socket_send@sendto", sockerrno);
		return SOCK_ERR_ESEND;
	}

	/* Return the number of bytes sent. */
	if (sent_len != NULL)
		*sent_len = bytes_sent;

	return SOCK_OK;
}

/**
 * Receive some data from a TCP socket file descriptor.
 *
 * @param sockfd   TCP Socket file descriptor.
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
tcp_err_t tcp_socket_recv(int sockfd, void *buf, size_t buf_len,
						  size_t *recv_len, bool peek) {
	ssize_t bytes_recv;

	/* Check if we have a valid file descriptor. */
	if (sockfd == INVALID_SOCKET)
		return SOCK_EVT_CONN_CLOSED;

	/* Try to read some information from a socket. */
	bytes_recv = recv(sockfd, buf, buf_len, (peek) ? MSG_PEEK : MSG_WAITALL);
	if (bytes_recv == SOCKET_ERROR) {
		/* Check if it's just the connection being abruptly shut down. */
#ifdef _WIN32
		if (sockerrno == WSAEBADF)
#else
		if (errno == EBADF)
#endif /* _WIN32 */
			return SOCK_EVT_CONN_SHUTDOWN;

		log_sockerrno("tcp_socket_recv@recv", sockerrno);
		return SOCK_ERR_ERECV;
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
 * Receive some data from a UDP socket file descriptor.
 *
 * @param sockfd    UDP Socket file descriptor.
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
 * @see recv
 */
tcp_err_t udp_socket_recv(int sockfd, void *buf, size_t buf_len,
						  struct sockaddr *sock_addr, socklen_t *sock_len,
						  size_t *recv_len, bool peek) {
	ssize_t bytes_recv;

	/* Check if we have a valid file descriptor. */
	if (sockfd == INVALID_SOCKET)
		return SOCK_EVT_CONN_CLOSED;

	/* Try to read some information from a socket. */
	bytes_recv = recvfrom(sockfd, buf, buf_len, (peek) ? MSG_PEEK : 0,
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
		log_sockerrno("udp_socket_recv@recvfrom", sockerrno);
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
		log_sockerrno("udp_socket_recv@recvfrom", sockerrno);
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
 * Closes a socket file descriptor.
 *
 * @param sockfd Socket file descriptor to be closed.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 */
tcp_err_t socket_close(int sockfd) {
	int ret;

	/* Check if we are even needed. */
	if (sockfd == INVALID_SOCKET)
		return SOCK_OK;

	/* Close the socket file descriptor. */
#ifdef _WIN32
	ret = closesocket(sockfd);
#else
	ret = close(sockfd);
#endif /* _WIN32 */

	/* Check for errors. */
	if (ret == SOCKET_ERROR) {
		log_sockerrno("socket_close@close", sockerrno);
		return SOCK_ERR_ECLOSE;
	}

	return SOCK_OK;
}

/**
 * Closes a socket file descriptor.
 *
 * @param sockfd Socket file descriptor to be closed.
 *
 * @return SOCK_OK if the socket was properly closed.
 *         SOCK_ERR_ESHUTDOWN if the socket failed to shutdown properly.
 *         SOCK_ERR_ECLOSE if the socket failed to close properly.
 */
tcp_err_t socket_shutdown(int sockfd) {
	int ret;

	/* Check if we are even needed. */
	if (sockfd == INVALID_SOCKET)
		return SOCK_OK;

	/* Shutdown the socket file descriptor. */
#ifdef _WIN32
	ret = shutdown(sockfd, SD_BOTH);
	if ((ret == SOCKET_ERROR) && (sockerrno != WSAENOTCONN)) {
		log_sockerrno("socket_shutdown@shutdown", sockerrno);
		return SOCK_ERR_ESHUTDOWN;
	}

	ret = closesocket(sockfd);
	if (ret == SOCKET_ERROR) {
		log_sockerrno("socket_shutdown@closesocket", sockerrno);
		return SOCK_ERR_ECLOSE;
	}
#else
	ret = shutdown(sockfd, SHUT_RDWR);
	if ((ret == SOCKET_ERROR) && (errno != ENOTCONN) && (errno != EINVAL)) {
		log_sockerrno("socket_shutdown@shutdown", sockerrno);
		return SOCK_ERR_ESHUTDOWN;
	}

	ret = close(sockfd);
	if (ret == SOCKET_ERROR) {
		log_sockerrno("socket_shutdown@close", sockerrno);
		return SOCK_ERR_ECLOSE;
	}
#endif /* _WIN32 */

	return SOCK_OK;
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
bool socket_itos(char **buf, struct sockaddr *sock_addr) {
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
	/* Convert our string to UTF-8 assigning it to the return value. */
	*buf = utf16_wcstombs(tmp);
#else
	/* Allocate space for our return string. */
	*buf = (char *)malloc((strlen(tmp) + 1) * sizeof(char));
	if (*buf == NULL) {
		perror("tcp_socket_itos@malloc");
		return false;
	}

	/* Copy our IP address over and return. */
	strcpy(*buf, tmp);
#endif /* _WIN32 */

	return true;
}

/**
 * Gets the IP address that the TCP server is currently bound to in a string
 * representation.
 *
 * @warning This function will allocate memory that must be free'd by you.
 *
 * @param server Server handle object.
 *
 * @return Server's IP address as a string or NULL if an error occurred.
 *
 * @see socket_itos
 */
char *tcp_server_get_ipstr(const server_t *server) {
	char *buf;

	/* Perform the conversion. */
	if (!socket_itos(&buf, (struct sockaddr *)&server->tcp.addr_in))
		return NULL;

	return buf;
}

/**
 * Gets the IP address that the UDP server is currently bound to in a string
 * representation.
 *
 * @warning This function will allocate memory that must be free'd by you.
 *
 * @param server Server handle object.
 *
 * @return Server's IP address as a string or NULL if an error occurred.
 *
 * @see socket_itos
 */
char *udp_server_get_ipstr(const server_t *server) {
	char *buf;

	/* Perform the conversion. */
	if (!socket_itos(&buf, (struct sockaddr *)&server->udp.addr_in))
		return NULL;

	return buf;
}

/**
 * Gets the IP address of a client connection in a string representation.
 * @warning This function will allocate memory that must be free'd by you.
 *
 * @param conn Server client connection handle object.
 *
 * @return Client's IP address as a string or NULL if an error occurred.
 *
 * @see socket_itos
 */
char *tcp_server_conn_get_ipstr(const server_conn_t *conn) {
	char *buf;

	/* Perform the conversion. */
	if (!socket_itos(&buf, (struct sockaddr *)&conn->addr))
		return NULL;

	return buf;
}

/**
 * Gets the IP address of a client's server in a string representation.
 * @warning This function will allocate memory that must be free'd by you.
 *
 * @param client Client handle object.
 *
 * @return Client's server IP address as a string or NULL if an error occurred.
 *
 * @see socket_itos
 */
char *tcp_client_get_ipstr(const tcp_client_t *client) {
	char *buf;

	/* Perform the conversion. */
	if (!socket_itos(&buf, (struct sockaddr *)&client->addr_in))
		return NULL;

	return buf;
}

/**
 * Prints out the contents of a buffer ready to be transferred over the network.
 * Keep in mind that this means that everything will be in network byte order.
 *
 * @param buf Network byte ordered buffer to be inspected.
 * @param len Length of the buffer.
 */
void socket_net_buffer(const void *buf, size_t len) {
	size_t i;
	const uint8_t *p;

	p = buf;
	for (i = 0; i < len; i++) {
		printf("%02X ", p[i]);
	}
}
