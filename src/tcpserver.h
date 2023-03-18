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

/* Initialization and destruction. */
server_err_t server_start(const char *addr, uint16_t port);
server_err_t server_stop(void);

/* Connection handling. */
int server_accept(struct sockaddr_storage *conn_addr);
server_err_t server_close(int sockfd);

#ifdef __cplusplus
}
#endif

#endif /* _TCPSERVER_H */
