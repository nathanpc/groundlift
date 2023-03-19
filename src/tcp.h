/**
 * tcp.h
 * TCP server/client that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _TCPSERVERCLIENT_H
#define _TCPSERVERCLIENT_H

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TCP error codes. */
typedef enum {
	TCP_EVT_CONN_CLOSED = -1,
	TCP_OK,
	TCP_ERR_ESOCKET,
	TCP_ERR_EBIND,
	TCP_ERR_ELISTEN,
	TCP_ERR_ECLOSE,
	TCP_ERR_ESEND,
	TCP_ERR_ERECV,
	TCP_ERR_UNKNOWN
} tcp_err_t;

/* Server handle. */
typedef struct {
	int sockfd;

	struct sockaddr_in addr_in;
	socklen_t addr_in_size;
} server_t;

/* Server client connection handle. */
typedef struct {
	int sockfd;

	struct sockaddr_storage addr;
	socklen_t addr_size;
} server_conn_t;

/* Initialization and destruction. */
server_t *tcp_server_new(const char *addr, uint16_t port);
void tcp_server_free(server_t *server);

/* Server lifecycle. */
tcp_err_t tcp_server_start(server_t *server);
tcp_err_t tcp_server_stop(server_t *server);

/* Connection handling. */
server_conn_t *tcp_server_conn_accept(const server_t *server);
tcp_err_t tcp_server_conn_send(const server_conn_t *conn, const void *buf, size_t len);
tcp_err_t tcp_server_conn_recv(const server_conn_t *conn, void *buf, size_t buf_len, size_t *recv_len, bool peek);
tcp_err_t tcp_server_conn_close(server_conn_t *conn);
void tcp_server_conn_free(server_conn_t *conn);

/* Direct socket interactions. */
tcp_err_t tcp_socket_send(int sockfd, const void *buf, size_t len, size_t *sent_len);
tcp_err_t tcp_socket_recv(int sockfd, void *buf, size_t buf_len, size_t *recv_len, bool peek);
tcp_err_t tcp_socket_close(int sockfd);
bool tcp_socket_itos(char **buf, const struct sockaddr *sock_addr);

/* Misc. utilities. */
char *tcp_server_get_ipstr(const server_t *server);
char *tcp_server_conn_get_ipstr(const server_conn_t *conn);

#ifdef __cplusplus
}
#endif

#endif /* _TCPSERVERCLIENT_H */
