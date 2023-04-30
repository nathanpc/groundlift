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
#include "sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure holding all of the information about a transfer's current progress.
 */
typedef struct {
	const char *fname;
	const char *bname;

	uint32_t chunks;
	uint32_t sent_chunk;

	uint64_t fsize;
	uint16_t csize;
} gl_client_progress_t;

/**
 * Client connected to server event callback function pointer type definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_conn_func)(const tcp_client_t *client);

/**
 * Client disconnected to server event callback function pointer type
 * definition.
 *
 * @param client Client connection handle object.
 */
typedef void (*gl_client_evt_disconn_func)(const tcp_client_t *client);

/**
 * Client connection request reply received event callback function pointer type
 * definition.
 *
 * @param fname    Name of the file that was uploaded.
 * @param accepted Has the connection request been accepted?
 */
typedef void (*gl_client_evt_conn_req_resp_func)(const char *fname, bool accepted);

/**
 * Client file upload progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 */
typedef void (*gl_client_evt_put_progress_func)(const gl_client_progress_t *progress);

/**
 * Client file upload succeeded event callback function pointer type
 * definition.
 *
 * @param fname Name of the file that was uploaded.
 */
typedef void (*gl_client_evt_put_succeed_func)(const char *fname);

/**
 * Discovered peers event callback function pointer type definition.
 *
 * @param name Hostname of the peer.
 * @param addr IP address information of the peer.
 */
typedef void (*gl_client_evt_discovery_peer_func)(const char *name, const struct sockaddr *addr);

/* Initialization and destruction. */
gl_err_t *gl_client_init(const char *addr, uint16_t port);
void gl_client_free(void);

/* Client connection lifecycle. */
gl_err_t *gl_client_connect(char *fname);
bool gl_client_disconnect(void);
gl_err_t *gl_client_thread_join(void);

/* Client interactions. */
gl_err_t *gl_client_send_packet(obex_packet_t *packet);
gl_err_t *gl_client_send_conn_req(const char *fname, bool *accepted);
gl_err_t *gl_client_send_put_file(const char *fname);

/* Discovery service. */
gl_err_t *gl_client_discover_peers(uint16_t port);
gl_err_t *gl_client_discovery_thread_join(void);

/* Getters and setters. */
tcp_client_t *gl_client_get(void);

/* Callbacks */
void gl_client_evt_conn_set(gl_client_evt_conn_func func);
void gl_client_evt_disconn_set(gl_client_evt_disconn_func func);
void gl_client_evt_conn_req_resp_set(gl_client_evt_conn_req_resp_func func);
void gl_client_evt_put_progress_set(gl_client_evt_put_progress_func func);
void gl_client_evt_put_succeed_set(gl_client_evt_put_succeed_func func);
void gl_client_evt_discovery_peer_set(gl_client_evt_discovery_peer_func func);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLIENT_H */
