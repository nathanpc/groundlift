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
#include <utils/threads.h>

#include "error.h"
#include "obex.h"
#include "sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure holding all of the information about a client's connection request.
 */
typedef struct {
	const char *hostname;
	const server_conn_t *conn;

	file_bundle_t *fb;
} gl_server_conn_req_t;

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
 * @param arg    Optional data set by the event handler setup.
 */
typedef void (*gl_server_evt_start_func)(const server_t *server, void *arg);

/**
 * Server accepted a connection event callback function pointer type definition.
 *
 * @param conn Server client connection handle object.
 * @param arg  Optional data set by the event handler setup.
 */
typedef void (*gl_server_conn_evt_accept_func)(const server_conn_t *conn,
											   void *arg);

/**
 * Connection closed event callback function pointer type definition.
 *
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_server_conn_evt_close_func)(void *arg);

/**
 * Server stopped closed event callback function pointer type definition.
 *
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_server_evt_stop_func)(void *arg);

/**
 * Client connection request event callback function pointer type definition.
 *
 * @param req Information about the client and its request.
 * @param arg Optional data set by the event handler setup.
 *
 * @return Return 0 to refuse the connection request. Anything else will be
 *         treated as accepting.
 */
typedef int (*gl_server_evt_client_conn_req_func)(
	const gl_server_conn_req_t *req, void *arg);

/**
 * File download progress event callback function pointer type definition.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
typedef void (*gl_server_conn_evt_download_progress_func)(
	const gl_server_conn_progress_t *progress, void *arg);

/**
 * File downloaded successfully event callback function pointer type definition.
 *
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
typedef void (*gl_server_conn_evt_download_success_func)(
	const file_bundle_t *fb, void *arg);

/**
 * Client connection states.
 */
typedef enum {
	CONN_STATE_CREATED = 0,
	CONN_STATE_RECV_FILES,
	CONN_STATE_CANCELLED,
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
		thread_t main;
		thread_t discovery;
	} threads;

	/* Mutexes */
	struct {
		thread_mutex_t server;
		thread_mutex_t conn;
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

	/* Event handler arguments. */
	struct {
		void *started;
		void *conn_accepted;
		void *conn_closed;
		void *stopped;
		void *transfer_requested;
		void *transfer_progress;
		void *transfer_success;
	} event_args;
} server_handle_t;

/* Initialization and destruction. */
server_handle_t *gl_server_new(void);
gl_err_t *gl_server_setup(server_handle_t *handle, const char *addr,
						  uint16_t port);
gl_err_t *gl_server_free(server_handle_t *handle);

/* Server lifecycle. */
gl_err_t *gl_server_start(server_handle_t *handle);
gl_err_t *gl_server_conn_close(server_handle_t *handle);
gl_err_t *gl_server_conn_destroy(server_handle_t *handle);
gl_err_t *gl_server_conn_cancel(server_handle_t *handle);
gl_err_t *gl_server_stop(server_handle_t *handle);
gl_err_t *gl_server_thread_join(server_handle_t *handle);

/* Discovery server lifecycle. */
gl_err_t *gl_server_discovery_start(server_handle_t *handle, uint16_t port);
gl_err_t *gl_server_discovery_thread_join(server_handle_t *handle);

/* Callbacks */
void gl_server_evt_start_set(server_handle_t *handle,
							 gl_server_evt_start_func func, void *arg);
void gl_server_conn_evt_accept_set(server_handle_t *handle,
								   gl_server_conn_evt_accept_func func,
								   void *arg);
void gl_server_conn_evt_close_set(server_handle_t *handle,
	gl_server_conn_evt_close_func func, void *arg);
void gl_server_evt_stop_set(server_handle_t *handle,
							gl_server_evt_stop_func func, void *arg);
void gl_server_evt_client_conn_req_set(server_handle_t *handle,
									   gl_server_evt_client_conn_req_func func,
									   void *arg);
void gl_server_conn_evt_download_progress_set(server_handle_t *handle,
	gl_server_conn_evt_download_progress_func func, void *arg);
void gl_server_conn_evt_download_success_set(server_handle_t *handle,
	gl_server_conn_evt_download_success_func func, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _GL_SERVER_H */
