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
#include <utils/filesystem.h>

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
	file_bundle_t *fb;
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
 * @param fb File bundle that was uploaded.
 *
 * @return Return 0 to refuse the connection request. Anything else will be
 *         treated as accepting.
 */
typedef int (*gl_server_evt_client_conn_req_func)(const file_bundle_t *fb);

/**
 * File download progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 */
typedef void (*gl_server_conn_evt_download_progress_func)(
	const gl_server_conn_progress_t *progress);

/**
 * File downloaded successfully event callback function pointer type definition.
 *
 * @param fb File bundle that was uploaded.
 */
typedef void (*gl_server_conn_evt_download_success_func)(
	const file_bundle_t *fb);

/**
 * Client connection states.
 */
typedef enum {
	CONN_STATE_CREATED = 0,
	CONN_STATE_RECV_FILES,
	CONN_STATE_ERROR
} conn_state_t;

/**
 * Server handle object.
 */
typedef struct {
	server_t *server;
	server_conn_t *conn;
	obex_opcodes_t expected_opcodes;

	/* Threads */
	struct {
		pthread_t *main;
		pthread_t *discovery;
	} threads;

	/* Mutexes */
	struct {
		pthread_mutex_t *server;
		pthread_mutex_t *conn;
	} mutexes;

	/* Event handlers. */
	struct {
		gl_server_evt_start_func started;
		gl_server_conn_evt_accept_func conn_accepted;
		gl_server_conn_evt_close_func conn_closed;
		gl_server_evt_stop_func stopped;
		gl_server_evt_client_conn_req_func transfer_requested;
		gl_server_conn_evt_download_progress_func transfer_progress;
		gl_server_conn_evt_download_success_func transfer_success;
	} events;
} server_handle_t;

/* Initialization and destruction. */
server_handle_t *gl_server_new(void);
gl_err_t *gl_server_setup(server_handle_t *handle, const char *addr,
						  uint16_t port);
gl_err_t *gl_server_free(server_handle_t *handle);

/* Server lifecycle. */
gl_err_t *gl_server_start(server_handle_t *handle);
gl_err_t *gl_server_conn_destroy(server_handle_t *handle);
gl_err_t *gl_server_stop(server_handle_t *handle);
gl_err_t *gl_server_thread_join(server_handle_t *handle);

/* Discovery server lifecycle. */
gl_err_t *gl_server_discovery_start(server_handle_t *handle, uint16_t port);
gl_err_t *gl_server_discovery_thread_join(server_handle_t *handle);

/* Callbacks */
void gl_server_evt_start_set(server_handle_t *handle,
							 gl_server_evt_start_func func);
void gl_server_conn_evt_accept_set(server_handle_t *handle,
								   gl_server_conn_evt_accept_func func);
void gl_server_conn_evt_close_set(server_handle_t *handle,
								  gl_server_conn_evt_close_func func);
void gl_server_evt_stop_set(server_handle_t *handle,
							gl_server_evt_stop_func func);
void gl_server_evt_client_conn_req_set(server_handle_t *handle,
									   gl_server_evt_client_conn_req_func func);
void gl_server_conn_evt_download_progress_set(server_handle_t *handle,
								gl_server_conn_evt_download_progress_func func);
void gl_server_conn_evt_download_success_set(server_handle_t *handle,
								gl_server_conn_evt_download_success_func func);

#ifdef __cplusplus
}
#endif

#endif /* _GL_SERVER_H */
