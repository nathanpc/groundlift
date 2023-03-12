/**
 * tcpserver.h
 * TCP server that forms the basis of the communication between nodes.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Server error codes. */
typedef enum {
	SERVER_OK = 0,
	SERVER_ERR_ESOCKET,
	SERVER_ERR_EBIND,
	SERVER_ERR_UNKNOWN
} server_err_t;

/* Initialization and destruction. */
server_err_t server_start(const char *addr, uint16_t port);
void server_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _TCPSERVER_H */
