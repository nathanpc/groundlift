/**
 * tcpserver.h
 * TCP server that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Server error codes. */
typedef enum {
	SERVER_OK = 0,
	SERVER_ERR_ESOCKET,
	SERVER_ERR_EBIND,
	SERVER_ERR_ELISTEN,
	SERVER_ERR_ECLOSE,
	SERVER_ERR_UNKNOWN
} server_err_t;

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
server_t *server_new(const char *addr, uint16_t port);
void server_free(server_t *server);

/* Server lifecycle. */
server_err_t server_start(server_t *server);
server_err_t server_stop(server_t *server);

/* Connection handling. */
server_conn_t *server_conn_accept(server_t *server);
server_err_t server_conn_close(server_conn_t *conn);
void server_conn_free(server_conn_t *conn);

/* Socket file descriptor handling. */
server_err_t server_socket_close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif /* _TCPSERVER_H */
