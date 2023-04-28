/**
 * server.h
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_SERVER_H
#define _GL_SERVER_H

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
	const char *fpath;
	const char *bname;

	uint32_t fsize;
	uint32_t recv_bytes;
} gl_server_conn_progress_t;

/**
 * Server started event callback function pointer type definition.
 *
 * @param server Server handle object.
 */
typedef void (*gl_server_evt_start_func)(const server_t *server);

/**
 * Server accepted a connection event callback function pointer type definition.
 *
 * @param conn Server client connection handle object.
 */
typedef void (*gl_server_conn_evt_accept_func)(const server_conn_t *conn);

/**
 * Connection closed event callback function pointer type definition.
 */
typedef void (*gl_server_conn_evt_close_func)(void);

/**
 * Server stopped closed event callback function pointer type definition.
 */
typedef void (*gl_server_evt_stop_func)(void);

/**
 * Client connection request event callback function pointer type definition.
 *
 * @param fname Name of the file to be downloaded.
 * @param fsize Size in bytes of the entire file being downloaded. Will be 0 if
 *              the client didn't send a file length.
 *
 * @return Return 0 to refuse the connection request. Anything else will be
 *         treated as accepting.
 */
typedef int (*gl_server_evt_client_conn_req_func)(const char *fname, uint32_t fsize);

/**
 * File download progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 */
typedef void (*gl_server_conn_evt_download_progress_func)(const gl_server_conn_progress_t *progress);

/**
 * File downloaded successfully event callback function pointer type definition.
 *
 * @param fpath Path of the downloaded file.
 */
typedef void (*gl_server_conn_evt_download_success_func)(const char *fpath);

/**
 * Client connection states.
 */
typedef enum {
	CONN_STATE_CREATED = 0,
	CONN_STATE_RECV_FILES,
	CONN_STATE_ERROR
} conn_state_t;

/* Initialization and destruction. */
bool gl_server_init(const char *addr, uint16_t port);
void gl_server_free(void);

/* Server lifecycle. */
bool gl_server_start(void);
tcp_err_t gl_server_conn_destroy(void);
bool gl_server_stop(void);
bool gl_server_thread_join(void);

/* Discovery server lifecycle. */
bool gl_server_discovery_start(uint16_t port);
bool gl_server_discovery_thread_join(void);

/* Server interactions. */
gl_err_t *gl_server_send_packet(obex_packet_t *packet);
gl_err_t *gl_server_handle_conn_req(const obex_packet_t *packet, bool *accepted);
gl_err_t *gl_server_handle_put_req(const obex_packet_t *init_packet, bool *running, conn_state_t *state);

/* Getters and setters. */
server_t *gl_server_get(void);

/* Callbacks */
void gl_server_evt_start_set(gl_server_evt_start_func func);
void gl_server_conn_evt_accept_set(gl_server_conn_evt_accept_func func);
void gl_server_conn_evt_close_set(gl_server_conn_evt_close_func func);
void gl_server_evt_stop_set(gl_server_evt_stop_func func);
void gl_server_evt_client_conn_req_set(gl_server_evt_client_conn_req_func func);
void gl_server_conn_evt_download_progress_set(gl_server_conn_evt_download_progress_func func);
void gl_server_conn_evt_download_success_set(gl_server_conn_evt_download_success_func func);

#ifdef __cplusplus
}
#endif

#endif /* _GL_SERVER_H */
