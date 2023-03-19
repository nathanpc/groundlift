/**
 * tcp.h
 * TCP server/client that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _TCPSERVERCLIENT_H
#define _TCPSERVERCLIENT_H

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TCP error codes. */
typedef enum {
	TCP_OK = 0,
	TCP_ERR_ESOCKET,
	TCP_ERR_EBIND,
	TCP_ERR_ELISTEN,
	TCP_ERR_ECLOSE,
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
server_conn_t *tcp_server_conn_accept(server_t *server);
tcp_err_t tcp_server_conn_close(server_conn_t *conn);
void tcp_server_conn_free(server_conn_t *conn);

/* Direct socket interactions. */
tcp_err_t tcp_socket_close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif /* _TCPSERVERCLIENT_H */
