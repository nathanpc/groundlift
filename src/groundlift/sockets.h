/**
 * sockets.h
 * Socket server/client that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _SOCKSERVERCLIENT_H
#define _SOCKSERVERCLIENT_H

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdshim.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif /* _WIN32 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
/* Shim things that Microsoft forgot to include in their implementation. */

#ifndef in_addr_t
typedef struct in_addr in_addr_t;
#endif /* in_addr_t */

#endif /* _WIN32 */

/* TCP error codes. */
typedef enum {
	SOCK_EVT_TIMEOUT = -3,
	SOCK_EVT_CONN_SHUTDOWN,
	SOCK_EVT_CONN_CLOSED,
	SOCK_OK,
	SOCK_ERR_ESOCKET,
	SOCK_ERR_ESETSOCKOPT,
	SOCK_ERR_EBIND,
	TCP_ERR_ELISTEN,
	SOCK_ERR_ECLOSE,
	SOCK_ERR_ESEND,
	SOCK_ERR_ERECV,
	TCP_ERR_ECONNECT,
	SOCK_ERR_ESHUTDOWN,
	TCP_ERR_UNKNOWN
} tcp_err_t;

/* Socket bundle handle. */
typedef struct {
	int sockfd;
	struct sockaddr_in addr_in;
	socklen_t addr_in_size;
} sock_bundle_t;

/* Server handle. */
typedef struct {
	sock_bundle_t tcp;
	sock_bundle_t udp;
} server_t;

/* Server client connection handle. */
typedef struct {
	int sockfd;
	struct sockaddr_storage addr;
	socklen_t addr_size;

	uint16_t packet_len;
} server_conn_t;

/* Client handle. */
typedef struct {
	int sockfd;
	struct sockaddr_in addr_in;
	socklen_t addr_in_size;

	uint16_t packet_len;
} tcp_client_t;

/* Initialization and destruction. */
server_t *sockets_server_new(const char *addr, uint16_t port);
tcp_client_t *tcp_client_new(const char *addr, uint16_t port);
void sockets_server_free(server_t *server);
void tcp_server_conn_free(server_conn_t *conn);
void tcp_client_free(tcp_client_t *client);

/* Server lifecycle. */
tcp_err_t sockets_server_start(server_t *server);
tcp_err_t sockets_server_stop(server_t *server);
tcp_err_t sockets_server_shutdown(server_t *server);

/* Discovery service. */
tcp_err_t udp_discovery_init(sock_bundle_t *sock, bool server,
							 in_addr_t inaddr, uint16_t port,
							 uint32_t timeout_ms);

/* Connection handling. */
server_conn_t *tcp_server_conn_accept(const server_t *server);
tcp_err_t tcp_client_connect(tcp_client_t *client);
tcp_err_t tcp_server_conn_send(const server_conn_t *conn, const void *buf,
							   size_t len);
tcp_err_t tcp_client_send(const tcp_client_t *client, const void *buf,
						  size_t len);
tcp_err_t tcp_server_conn_recv(const server_conn_t *conn, void *buf,
							   size_t buf_len, size_t *recv_len, bool peek);
tcp_err_t tcp_client_recv(const tcp_client_t *client, void *buf, size_t buf_len,
						  size_t *recv_len, bool peek);
tcp_err_t tcp_server_conn_shutdown(server_conn_t *conn);
tcp_err_t tcp_client_close(tcp_client_t *client);
tcp_err_t tcp_client_shutdown(tcp_client_t *client);

/* Direct socket interactions. */
socklen_t socket_setup_inaddr(struct sockaddr_in *addr, in_addr_t inaddr,
							  uint16_t port);
socklen_t socket_setup(struct sockaddr_in *addr, const char *ipaddr,
					   uint16_t port);
tcp_err_t tcp_socket_send(int sockfd, const void *buf, size_t len,
						  size_t *sent_len);
tcp_err_t udp_socket_send(int sockfd, const void *buf, size_t len,
						  const struct sockaddr *sock_addr, socklen_t sock_len,
						  size_t *sent_len);
tcp_err_t tcp_socket_recv(int sockfd, void *buf, size_t buf_len,
						  size_t *recv_len, bool peek);
tcp_err_t udp_socket_recv(int sockfd, void *buf, size_t buf_len,
						  struct sockaddr *sock_addr, socklen_t *sock_len,
						  size_t *recv_len, bool peek);
tcp_err_t socket_close(int sockfd);
tcp_err_t socket_shutdown(int sockfd);
bool socket_itos(char **buf, struct sockaddr *sock_addr);

/* Misc. utilities. */
char *tcp_server_get_ipstr(const server_t *server);
char *udp_server_get_ipstr(const server_t *server);
char *tcp_server_conn_get_ipstr(const server_conn_t *conn);
char *tcp_client_get_ipstr(const tcp_client_t *client);

/* Debugging */
void socket_print_net_buffer(const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _SOCKSERVERCLIENT_H */
