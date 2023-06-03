/**
 * server.c
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <string.h>
#include <utils/filesystem.h>
#include <utils/logging.h>
#include <utils/threads.h>
#include <utils/utf16.h>

#include "conf.h"
#include "defaults.h"
#include "error.h"
#include "obex.h"

/* Private methods. */
obex_opcodes_t *gl_server_expected_opcodes(server_handle_t *handle,
										   conn_state_t state);
gl_err_t *gl_server_packet_file_info(const obex_packet_t *packet,
									 file_bundle_t **fb);
gl_err_t *gl_server_send_packet(server_handle_t *handle, obex_packet_t *packet);
gl_err_t *gl_server_handle_conn_req(server_handle_t *handle,
									const obex_packet_t *packet,
									bool *accepted);
gl_err_t *gl_server_handle_put_req(server_handle_t *handle,
								   const obex_packet_t *init_packet,
								   bool *running, conn_state_t *state);
void *server_thread_func(void *handle_ptr);
void *discovery_thread_func(void *handle_ptr);

/**
 * Allocates a brand new server handle object and populates it with some sane
 * defaults.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated server handle object or NULL if an error occurred.
 *
 * @see gl_server_setup
 * @see gl_server_free
 */
server_handle_t *gl_server_new(void) {
	server_handle_t *handle;

	/* Allocate some memory for our handle object. */
	handle = (server_handle_t *)malloc(sizeof(server_handle_t));
	if (handle == NULL)
		return NULL;

	/* Set some sane defaults. */
	handle->server = NULL;
	handle->conn = NULL;
	handle->expected_opcodes = 0;

	handle->threads.main = NULL;
	handle->threads.discovery = NULL;

	handle->mutexes.server = thread_mutex_new();
	handle->mutexes.conn = thread_mutex_new();

	handle->events.started = NULL;
	handle->events.conn_accepted = NULL;
	handle->events.conn_closed = NULL;
	handle->events.stopped = NULL;
	handle->events.transfer_requested = NULL;
	handle->events.transfer_progress = NULL;
	handle->events.transfer_success = NULL;

	handle->event_args.started = NULL;
	handle->event_args.conn_accepted = NULL;
	handle->event_args.conn_closed = NULL;
	handle->event_args.stopped = NULL;
	handle->event_args.transfer_requested = NULL;
	handle->event_args.transfer_progress = NULL;
	handle->event_args.transfer_success = NULL;

	return handle;
}

/**
 * Sets everything up for the server handle to be usable.
 *
 * @param handle Server handle object.
 * @param addr   Address to listen on.
 * @param port   TCP port to listen on for file transfers.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_start
 */
gl_err_t *gl_server_setup(server_handle_t *handle, const char *addr,
						  uint16_t port) {
	/* Get a server handle. */
	handle->server = sockets_server_new(addr, port);
	if (handle->server == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_SOCKET,
			EMSG("Failed to allocate and setup the server sockets"));
	}

	return NULL;
}

/**
 * Frees up any resources allocated by the server. Shutting down the server
 * previously to calling this function isn't required.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_stop
 */
gl_err_t *gl_server_free(server_handle_t *handle) {
	gl_err_t *err;

	/* Do we even have a handle? */
	if (handle == NULL)
		return NULL;

	/* Stop the servers and free up any allocated resources. */
	err = gl_server_stop(handle);
	if (err)
		gl_error_print(err);
	sockets_server_free(handle->server);
	handle->server = NULL;

	/* Join the discovery service's thread and free its mutex. */
	err = gl_server_discovery_thread_join(handle);
	if (err)
		gl_error_print(err);

	/* Join the server thread. */
	err = gl_server_thread_join(handle);

	/* Free our mutexes. */
	thread_mutex_free(&handle->mutexes.server);
	thread_mutex_free(&handle->mutexes.conn);

	return err;
}

/**
 * Starts the server up.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_thread_join
 * @see gl_server_stop
 */
gl_err_t *gl_server_start(server_handle_t *handle) {
	int ret;

	/* Check if we already have the discovery thread running. */
	if (handle->threads.main != NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_THREAD,
							EMSG("Server's main thread already created"));
	}

	/* Allocate some memory for our main server thread. */
	handle->threads.main = thread_new();
	if (handle->threads.main == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to allocate the server thread"));
	}

	/* Create the server thread. */
	ret = thread_create(handle->threads.main, server_thread_func,
						(void *)handle);
	if (ret) {
		handle->threads.main = NULL;
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
								  EMSG("Failed to start the server thread"));
	}

	return NULL;
}

/**
 * Closes and frees up any resources associated with the current active
 * connection to the server.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_stop
 * @see tcp_server_conn_close
 */
gl_err_t *gl_server_conn_destroy(server_handle_t *handle) {
	gl_err_t *err;
	tcp_err_t tcp_err;

	/* Do we even need to do stuff? */
	if ((handle == NULL) || (handle->conn == NULL))
		return NULL;

	/* Close the connection and free up any resources allocated. */
	tcp_err = tcp_server_conn_shutdown(handle->conn);
	tcp_server_conn_free(handle->conn);
	handle->conn = NULL;

	/* Check for errors. */
	err = NULL;
	if (tcp_err > SOCK_OK) {
		err = gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to shutdown the server's client connection socket"));
	}

	return err;
}

/**
 * Stops the running server if needed.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_thread_join
 * @see gl_server_free
 * @see gl_server_conn_destroy
 */
gl_err_t *gl_server_stop(server_handle_t *handle) {
	gl_err_t *err;
	tcp_err_t tcp_err;

	/* Do we even need to stop it? */
	if ((handle == NULL) || (handle->server == NULL))
		return NULL;

	/* Destroy any active connections. */
	thread_mutex_lock(handle->mutexes.conn);
	err = gl_server_conn_destroy(handle);
	if (err)
		gl_error_print(err);
	thread_mutex_unlock(handle->mutexes.conn);

	/* Shut the thing down. */
	thread_mutex_lock(handle->mutexes.server);
	tcp_err = sockets_server_shutdown(handle->server);
	thread_mutex_unlock(handle->mutexes.server);

	/* Check for errors. */
	err = NULL;
	if (tcp_err > SOCK_OK) {
		err = gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to shutdown the main server/discovery sockets"));
	}

	return err;
}

/**
 * Waits for the server thread to return.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_thread_join(server_handle_t *handle) {
	gl_err_t *err;

	/* Do we even need to do something? */
	if (handle == NULL)
		return NULL;

	/* Join the thread back into us. */
	if (thread_join(&handle->threads.main, (void **)&err) > THREAD_OK) {
		log_errno(LOG_ERROR, "gl_server_thread_join@thread_join");
	}

	return err;
}

/**
 * Starts the server's discovery service up.
 *
 * @param handle Server handle object.
 * @param port   Port to bind the discovery service to.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_server_discovery_thread_join
 * @see gl_server_stop
 */
gl_err_t *gl_server_discovery_start(server_handle_t *handle, uint16_t port) {
	tcp_err_t err;
	int ret;

	/* Check if we already have the discovery thread running. */
	if (handle->threads.discovery != NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_THREAD,
							EMSG("Discovery server thread already created"));
	}

	/* Allocate our thread object. */
	handle->threads.discovery = thread_new();
	if (handle->threads.discovery == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to allocate the server thread"));
	}

	/* Initialize discovery service. */
	thread_mutex_lock(handle->mutexes.server);
	err = udp_discovery_init(&handle->server->udp, true, INADDR_ANY, port, 0);
	thread_mutex_unlock(handle->mutexes.server);
	if (err) {
		free(handle->threads.discovery);
		handle->threads.discovery = NULL;

		return gl_error_new(ERR_TYPE_TCP, (int8_t)err,
			EMSG("Failed to initialize the discovery server socket"));
	}

	/* Create the server thread. */
	ret = thread_create(handle->threads.discovery, discovery_thread_func,
						(void *)handle);
	if (ret) {
		handle->threads.discovery = NULL;
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to start the discovery server thread"));
	}

	return NULL;
}

/**
 * Waits for the server thread to return.
 *
 * @param handle Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_discovery_thread_join(server_handle_t *handle) {
	gl_err_t *err;

	/* Do we even need to do something? */
	if (handle == NULL)
		return NULL;

	/* Join the thread back into us. */
	if (thread_join(&handle->threads.discovery, (void **)&err) > THREAD_OK) {
		log_errno(LOG_ERROR, "gl_server_discovery_thread_join@thread_join");
	}

	return err;
}

/**
 * Sends an OBEX packet to the client.
 *
 * @param handle Server handle object.
 * @param packet OBEX packet object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_send_packet(server_handle_t *handle,
								obex_packet_t *packet) {
	gl_err_t *err;

	thread_mutex_lock(handle->mutexes.conn);
	err = obex_net_packet_send(handle->conn->sockfd, packet);
	thread_mutex_unlock(handle->mutexes.conn);

	return err;
}

/**
 * Handles a client's connection request.
 *
 * @param handle   Server handle object.
 * @param packet   OBEX packet object.
 * @param accepted Pointer to a boolean that will store a flag of whether the
 *                 connection request was accepted.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_handle_conn_req(server_handle_t *handle,
									const obex_packet_t *packet,
									bool *accepted) {
	gl_err_t *err;
	obex_packet_t *resp;
	const obex_header_t *header;
	gl_server_conn_req_t req;

	/* Get the file information. */
	err = gl_server_packet_file_info(packet, &req.fb);
	if (err)
		return err;

	/* Get some information about the client. */
	req.conn = handle->conn;
	req.hostname = NULL;
	header = obex_packet_header_find(packet, OBEX_HEADER_EXT_HOSTNAME);
	if (header)
		req.hostname = header->value.string.text;

	/* Trigger the connection requested event. */
	*accepted = true;
	if (handle->events.transfer_requested != NULL) {
		*accepted = handle->events.transfer_requested(&req,
			handle->event_args.transfer_requested);
	}

	/* Set our packet length if needed. */
	thread_mutex_lock(handle->mutexes.conn);
	if (packet->params[2].value.uint16 < handle->conn->packet_len)
		handle->conn->packet_len = packet->params[2].value.uint16;
	thread_mutex_unlock(handle->mutexes.conn);

	/* Initialize reply packet. */
	if (*accepted) {
		resp = obex_packet_new_success(true, true);
	} else {
		resp = obex_packet_new_unauthorized(true);
	}

	/* Send reply. */
	err = gl_server_send_packet(handle, resp);
	obex_packet_free(resp);

	return err;
}

/**
 * Handles a file transfer from a client.
 *
 * @param handle      Server handle object.
 * @param init_packet OBEX packet object.
 * @param running     Pointer to a boolean that will store a flag of whether the
 *                    connection should continue running.
 * @param state       Pointer to the variable that's holding the current state
 *                    of the client's connection.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_handle_put_req(server_handle_t *handle,
								   const obex_packet_t *init_packet,
								   bool *running, conn_state_t *state) {
	gl_err_t *err;
	gl_server_conn_progress_t progress;
	obex_packet_t *packet;
	obex_packet_t *resp;
	obex_opcodes_t expected;
	file_bundle_t *fb;
	char *fpath;
	FILE *fh;

	/* Initialize some variables. */
	expected = OBEX_OPCODE_PUT;

	/* Get the file name and size. */
	err = gl_server_packet_file_info(init_packet, &fb);
	if (err)
		return err;
	if (fb->size == 0)
		log_printf(LOG_WARNING, "PUT request doesn't include a file length\n");

	/* Get the path to save the downloaded file to. */
	fpath = path_build_download(conf_get_download_dir(), fb->base);
	if (fpath == NULL) {
		file_bundle_free(fb);
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
								  EMSG("Failed to build download file path"));
	}

	/* Switch around the file bundle path to the local one. */
	if (!file_bundle_set_name(fb, fpath)) {
		file_bundle_free(fb);
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to set file bundle to the download file path"));
	}
	free(fpath);
	fpath = NULL;

	/* Get the basename of the file and populate the information structure. */
	progress.fb = fb;
	progress.recv_bytes = 0;

	/* Open the file for download. */
	fh = file_open(fb->name, "wb");
	if (fh == NULL) {
		*running = false;
		*state = CONN_STATE_ERROR;
		file_bundle_free(fb);

		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
							EMSG("Failed to open the file for download"));
	}

	/* Write the chunk of data to the file. */
	progress.recv_bytes += init_packet->body_length;
	if (file_write(fh, init_packet->body, init_packet->body_length) < 0) {
		*running = false;
		*state = CONN_STATE_ERROR;
		file_bundle_free(fb);

		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
			EMSG("Failed to write initial data to the downloaded file"));
	}

	/* Update the progress of the current transfer via an event. */
	if (handle->events.transfer_progress != NULL) {
		handle->events.transfer_progress(&progress,
			handle->event_args.transfer_progress);
	}

	/* Initialize reply packet. */
	if (OBEX_IS_FINAL_OPCODE(init_packet->opcode)) {
		resp = obex_packet_new_success(true, false);
	} else {
		resp = obex_packet_new_continue(true);
	}

	/* Send reply. */
	err = gl_server_send_packet(handle, resp);
	obex_packet_free(resp);

	/* Was this transfer just a single packet long? */
	if (OBEX_IS_FINAL_OPCODE(init_packet->opcode)) {
		/* Close the downloaded file handle. */
		file_close(fh);

		/* Trigger the downloaded success event. */
		if (handle->events.transfer_success != NULL) {
			handle->events.transfer_success(fb,
				handle->event_args.transfer_success);
		}

		/* Free the file bundle. */
		file_bundle_free(fb);

		return err;
	}

	/* Receive the rest of the file. */
	while ((packet = obex_net_packet_recv(
				handle->conn->sockfd, &expected, false)) != NULL) {
		/* Check if we got an invalid packet. */
		if (packet == obex_invalid_packet)
			break;

		/* Write the chunk of data to the file. */
		progress.recv_bytes += packet->body_length;
		if (file_write(fh, packet->body, packet->body_length) < 0) {
			*running = false;
			*state = CONN_STATE_ERROR;
			obex_packet_free(packet);
			file_bundle_free(fb);

			return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
				EMSG("Failed to write chunked data to the downloaded file"));
		}

		/* Update the progress of the current transfer via an event. */
		if (handle->events.transfer_progress != NULL) {
			handle->events.transfer_progress(&progress,
				handle->event_args.transfer_progress);
		}

		/* Initialize reply packet. */
		if (OBEX_IS_FINAL_OPCODE(packet->opcode)) {
			resp = obex_packet_new_success(true, false);
		} else {
			resp = obex_packet_new_continue(true);
		}

		/* Send reply. */
		err = gl_server_send_packet(handle, resp);
		obex_packet_free(resp);

		/* Has the transfer ended? */
		if (OBEX_IS_FINAL_OPCODE(packet->opcode)) {
			/* Free the received packet and close the downloaded file handle. */
			obex_packet_free(packet);
			file_close(fh);

			/* Trigger the downloaded success event. */
			if (handle->events.transfer_success != NULL) {
				handle->events.transfer_success(fb,
					handle->event_args.transfer_success);
			}

			/* Free the file bundle. */
			file_bundle_free(fb);

			return err;
		}

		/* Free our resources. */
		obex_packet_free(packet);
	}

	/* Free any resources and return the error. */
	file_bundle_free(fb);
	return gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_RECV,
		EMSG("An invalid packet was received during a chunked file transfer"));
}

/**
 * Server thread function.
 *
 * @param handle_ptr Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
void *server_thread_func(void *handle_ptr) {
	server_handle_t *handle;
	tcp_err_t tcp_err;
	gl_err_t *gl_err;

	/* Initialize variables. */
	handle = (server_handle_t *)handle_ptr;
	gl_err = NULL;

	/* Start the server and listen for incoming connections. */
	thread_mutex_lock(handle->mutexes.server);
	tcp_err = sockets_server_start(handle->server);
	thread_mutex_unlock(handle->mutexes.server);
	switch (tcp_err) {
		case SOCK_OK:
			break;
		case SOCK_ERR_ESOCKET:
			return gl_error_new(ERR_TYPE_TCP, SOCK_ERR_ESOCKET,
				EMSG("Server failed to create a socket"));
		case SOCK_ERR_EBIND:
			return gl_error_new(ERR_TYPE_TCP, SOCK_ERR_EBIND,
				EMSG("Server failed to bind to a socket"));
		case TCP_ERR_ELISTEN:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ELISTEN,
				EMSG("Server failed to listen on a socket"));
		default:
			return gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
				EMSG("tcp_server_start returned a weird error code"));
	}

	/* Trigger the server started event. */
	if (handle->events.started != NULL)
		handle->events.started(handle->server, handle->event_args.started);

	/* Accept incoming connections. */
	while ((handle->conn = tcp_server_conn_accept(handle->server)) != NULL) {
		obex_packet_t *packet;
		conn_state_t state;
		bool running;
		bool has_params;

		/* Check if the connection socket is valid. */
		if (handle->conn->sockfd == -1) {
			gl_server_conn_destroy(handle);
			break;
		}

		/* Trigger the connection accepted event. */
		if (handle->events.conn_accepted != NULL) {
			handle->events.conn_accepted(handle->conn,
				handle->event_args.conn_accepted);
		}

		/* Read packets until the connection is closed or an error occurs. */
		state = CONN_STATE_CREATED;
		running = true;
		has_params = true;
		while (((packet = obex_net_packet_recv(handle->conn->sockfd,
				 gl_server_expected_opcodes(handle, state),
				 has_params)) != NULL) && running) {
			/* Check if we got an invalid packet. */
			if (packet == obex_invalid_packet) {
				gl_err = gl_error_new(ERR_TYPE_GL, GL_ERR_INVALID_STATE_OPCODE,
					EMSG("Invalid opcode for the current connection state"));
				gl_error_print(gl_err);
				gl_error_free(gl_err);

				continue;
			}

			/* Handle operations for the different states. */
			switch (state) {
				case CONN_STATE_CREATED:
					if (packet->opcode == OBEX_OPCODE_CONNECT) {
						/* Got a connection request packet. */
						gl_err = gl_server_handle_conn_req(
							handle, packet, &running);
						if (running) {
							state = CONN_STATE_RECV_FILES;
							has_params = false;
						}
					}
					break;
				case CONN_STATE_RECV_FILES:
					/* Receiving files from the client. */
					if (OBEX_IGNORE_FINAL_BIT(packet->opcode)
							== OBEX_OPCODE_PUT) {
						/* Received a chunk of a file. */
						gl_err = gl_server_handle_put_req(
							handle, packet, &running, &state);
					}
					break;
				case CONN_STATE_ERROR:
					running = false;
					break;
				default:
					/* Invalid state. */
					gl_err = gl_error_new(ERR_TYPE_GL, GL_ERR_UNHANDLED_STATE,
						EMSG("Unhandled state condition"));
					running = false;
					break;
			}

			/* Make sure we free our packet. */
			obex_packet_free(packet);
			packet = NULL;
		}

		/* Close the connection and free up any resources. */
		obex_packet_free(packet);
		thread_mutex_lock(handle->mutexes.conn);
		gl_server_conn_destroy(handle);
		thread_mutex_unlock(handle->mutexes.conn);

		/* Trigger the connection closed event. */
		if (handle->events.conn_closed != NULL)
			handle->events.conn_closed(handle->event_args.conn_closed);
	}

	/* Stop the server and free up any resources. */
	gl_server_stop(handle);

	/* Trigger the server stopped event. */
	if (handle->events.stopped != NULL)
		handle->events.stopped(handle->event_args.stopped);

	return (void *)gl_err;
}

/**
 * Server discovery service thread function.
 *
 * @param handle_ptr Server handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
void *discovery_thread_func(void *handle_ptr) {
	server_handle_t *handle;
	gl_err_t *gl_err;
	obex_packet_t *packet;
	obex_opcodes_t op;

	/* Initialize variables. */
	handle = (server_handle_t *)handle_ptr;
	gl_err = NULL;
	op = OBEX_SET_FINAL_BIT(OBEX_OPCODE_GET);

	/* Listen for discovery packets. */
	while ((packet = obex_net_packet_recvfrom(&handle->server->udp, &op, false))
			!= NULL) {
		obex_packet_t *reply;
		const obex_header_t *header;

		/* Check if we got an invalid packet. */
		if (packet == obex_invalid_packet)
			continue;

		/* Check if the packet is meant for discovery. */
		header = obex_packet_header_find(packet, OBEX_HEADER_NAME);
		if ((header == NULL) ||
				(wcscmp(L"DISCOVER", header->value.wstring.text) != 0)) {
			log_printf(LOG_ERROR, "server_discovery_thread_func: Discovery "
				"packet name not \"DISCOVER\" or no name was supplied.\n");
			obex_packet_free(packet);

			continue;
		}

		/* Create the response packet. */
		reply = obex_packet_new_success(true, false);
		if (reply == NULL) {
			log_printf(LOG_FATAL, "server_discovery_thread_func: Failed to "
					   "create response packet.\n");
			obex_packet_free(packet);

			continue;
		}

		/* Add the hostname header to the packet. */
		if (!obex_packet_header_add_hostname(reply)) {
			log_printf(LOG_FATAL, "server_discovery_thread_func: Failed to "
					   "append hostname header to the packet.\n");
			obex_packet_free(reply);
			obex_packet_free(packet);

			continue;
		}

		/* Send reply and free our resources. */
		gl_err = obex_net_packet_sendto(handle->server->udp, reply);
		obex_packet_free(reply);
		obex_packet_free(packet);

		/* Check for sending errors. */
		if (gl_err)
			break;
	}

	return (void *)gl_err;
}

/**
 * Gets the expected opcodes for a given connection state.
 *
 * @param handle Server handle object.
 * @param state Connection state.
 *
 * @return Expected opcodes for the connection state.
 */
obex_opcodes_t *gl_server_expected_opcodes(server_handle_t *handle,
										   conn_state_t state) {
	/* Set the internal expected opcode variable accordingly. */
	switch (state) {
		case CONN_STATE_CREATED:
			handle->expected_opcodes = OBEX_OPCODE_CONNECT;
			break;
		case CONN_STATE_RECV_FILES:
			handle->expected_opcodes = OBEX_OPCODE_PUT;
			break;
		default:
			handle->expected_opcodes = 0;
			break;
	}

	/* Handle the "catch all" expected value. */
	if (handle->expected_opcodes == 0)
		return NULL;

	return &handle->expected_opcodes;
}

/**
 * Gets information about a file being transferred from an OBEX packet using the
 * standard NAME and LENGTH fields.
 *
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param packet OBEX packet object to extract the information from.
 * @param fb     Pointer to a file bundle object that will store the file
 *               information. Set to NULL on error. (Allocated by this function)
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_server_packet_file_info(const obex_packet_t *packet,
									 file_bundle_t **fb) {
	const obex_header_t *header;
	char *fname;

	/* Create the file bundle. */
	*fb = file_bundle_new_empty();
	if (*fb == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to create a file bundle for packet file information"));
	}

	/* Get the file name. */
	header = obex_packet_header_find(packet, OBEX_HEADER_NAME);
	if ((header == NULL) || (header->value.wstring.text == NULL)) {
		fname = strdup("Unnamed");
	} else {
		fname = utf16_wcstombs(header->value.wstring.text);
	}

	/* Set the file name of the bundle. */
	if (!file_bundle_set_name(*fb, fname)) {
		file_bundle_free(*fb);
		free(fname);

		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to set bundle name from packet file information"));
	}

	/* Free up temporary resources. */
	free(fname);
	fname = NULL;

	/* Get the file size. */
	header = obex_packet_header_find(packet, OBEX_HEADER_LENGTH);
	(*fb)->size = (header) ? header->value.word32 : 0;

	return NULL;
}

/**
 * Sets the Started event callback function.
 *
 * @param handle Server handle object.
 * @param func   Started event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_evt_start_set(server_handle_t *handle,
							 gl_server_evt_start_func func, void *arg) {
	handle->events.started = func;
	handle->event_args.started = arg;
}

/**
 * Sets the Connection Accepted event callback function.
 *
 * @param handle Server handle object.
 * @param func   Connection Accepted event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_conn_evt_accept_set(server_handle_t *handle,
								   gl_server_conn_evt_accept_func func,
								   void *arg) {
	handle->events.conn_accepted = func;
	handle->event_args.conn_accepted = arg;
}

/**
 * Sets the Connection Closed event callback function.
 *
 * @param handle Server handle object.
 * @param func   Connection Closed event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_conn_evt_close_set(server_handle_t *handle,
		gl_server_conn_evt_close_func func, void *arg) {
	handle->events.conn_closed = func;
	handle->event_args.conn_closed = arg;
}

/**
 * Sets the Stopped event callback function.
 *
 * @param handle Server handle object.
 * @param func   Stopped event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_evt_stop_set(server_handle_t *handle,
							gl_server_evt_stop_func func, void *arg) {
	handle->events.stopped = func;
	handle->event_args.stopped = arg;
}

/**
 * Sets the Client Connection Requested event callback function.
 *
 * @param handle Server handle object.
 * @param func   Client Connection Requested event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_evt_client_conn_req_set(server_handle_t *handle,
									   gl_server_evt_client_conn_req_func func,
									   void *arg) {
	handle->events.transfer_requested = func;
	handle->event_args.transfer_requested = arg;
}

/**
 * Sets the File Download Progress event callback function.
 *
 * @param handle Server handle object.
 * @param func   File Download Progress event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_conn_evt_download_progress_set(server_handle_t *handle,
		gl_server_conn_evt_download_progress_func func, void *arg) {
	handle->events.transfer_progress = func;
	handle->event_args.transfer_progress = arg;
}

/**
 * Sets the File Downloaded Successfully event callback function.
 *
 * @param handle Server handle object.
 * @param func   File Downloaded Successfully event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_server_conn_evt_download_success_set(server_handle_t *handle,
		gl_server_conn_evt_download_success_func func, void *arg) {
	handle->events.transfer_success = func;
	handle->event_args.transfer_success = arg;
}
