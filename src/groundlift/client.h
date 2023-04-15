/**
 * client.h
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLIENT_H
#define _GL_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "obex.h"
#include "tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Client connected to server event callback function pointer type definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_conn_func)(const tcp_client_t *client);

/**
 * Server connection closed gracefully event callback function pointer type
 * definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_close_func)(const tcp_client_t *client);

/**
 * Client disconnected to server event callback function pointer type
 * definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_disconn_func)(const tcp_client_t *client);

/**
 * Client connection request was accepted event callback function pointer type
 * definition.
 */
typedef void (*gl_client_evt_conn_req_accepted_func)(void);

/* Initialization and destruction. */
bool gl_client_init(const char *addr, uint16_t port);
void gl_client_free(void);

/* Client connection lifecycle. */
bool gl_client_connect(char *fname);
bool gl_client_disconnect(void);
bool gl_client_thread_join(void);

/* Client interactions. */
gl_err_t *gl_client_send_packet(obex_packet_t *packet);
gl_err_t *gl_client_send_conn_req(bool *accepted);
gl_err_t *gl_client_send_put_file(const char *fname, uint16_t psize);

/* Getters and setters. */
tcp_client_t *gl_client_get(void);

/* Callbacks */
void gl_client_evt_conn_set(gl_client_evt_conn_func func);
void gl_client_evt_close_set(gl_client_evt_close_func func);
void gl_client_evt_disconn_set(gl_client_evt_disconn_func func);
void gl_client_evt_conn_req_accepted_set(gl_client_evt_conn_req_accepted_func func);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
