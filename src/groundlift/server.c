/**
 * server.c
 * GroundLift server common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <pthread.h>
#include <string.h>

#include "defaults.h"
#include "error.h"
#include "fileutils.h"
#include "obex.h"
#include "utf16utils.h"

/* Private variables. */
static server_t *m_server;
static server_conn_t *m_conn;
static pthread_t *m_server_thread;
static pthread_mutex_t *m_server_mutex;
static pthread_mutex_t *m_conn_mutex;

/* Function callbacks. */
static gl_server_evt_start_func evt_server_start_cb_func;
static gl_server_conn_evt_accept_func evt_server_conn_accept_cb_func;
static gl_server_conn_evt_close_func evt_server_conn_close_cb_func;
static gl_server_evt_stop_func evt_server_stop_cb_func;
static gl_server_evt_client_conn_req_func evt_server_client_conn_req_cb_func;
static gl_server_conn_evt_download_success_func evt_server_conn_download_success_cb_func;

/* Private methods. */
void *server_thread_func(void *args);

/**
 * Initializes everything related to the server to a known clean state.
 *
 * @param addr Address to listen on.
 * @param port Port to listen on.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_start
 */
bool gl_server_init(const char *addr, uint16_t port) {
	/* Ensure we have everything in a known clean state. */
	m_conn = NULL;
	m_server_thread = (pthread_t *)malloc(sizeof(pthread_t));
	evt_server_start_cb_func = NULL;
	evt_server_conn_accept_cb_func = NULL;
	evt_server_conn_close_cb_func = NULL;
	evt_server_stop_cb_func = NULL;
	evt_server_client_conn_req_cb_func = NULL;
	evt_server_conn_download_success_cb_func = NULL;

	/* Initialize our mutexes. */
	m_server_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_server_mutex, NULL);
	m_conn_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_conn_mutex, NULL);

	/* Get a server handle. */
	m_server = tcp_server_new(addr, port);
	if (m_server == NULL)
		return false;

	/* Get the default download directory. */
	m_server->download_dir = dir_defaults_downloads();

	return true;
}

/**
 * Frees up any resources allocated by the server. Shutting down the server
 * previously to calling this function isn't required.
 *
 * @see gl_server_stop
 */
void gl_server_free(void) {
	/* Stop the server and free up any allocated resources. */
	gl_server_stop();
	tcp_server_free(m_server);
	m_server = NULL;

	/* Join the server thread. */
	gl_server_thread_join();

	/* Free our server mutex. */
	if (m_server_mutex) {
		free(m_server_mutex);
		m_server_mutex = NULL;
	}

	/* Free our connection mutex. */
	if (m_conn_mutex) {
		free(m_conn_mutex);
		m_conn_mutex = NULL;
	}
}

/**
 * Starts the server up.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_thread_join
 * @see gl_server_stop
 */
bool gl_server_start(void) {
	int ret;

	/* Create the server thread. */
	ret = pthread_create(m_server_thread, NULL, server_thread_func, NULL);
	if (ret)
		m_server_thread = NULL;

	return ret == 0;
}

/**
 * Stops the running server if needed.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_server_thread_join
 * @see gl_server_free
 * @see gl_server_conn_destroy
 */
bool gl_server_stop(void) {
	tcp_err_t err;

	/* Do we even need to stop it? */
	if (m_server == NULL)
		return true;

	/* Destroy any active connections. */
	pthread_mutex_lock(m_conn_mutex);
	gl_server_conn_destroy();
	pthread_mutex_unlock(m_conn_mutex);

	/* Shut the thing down. */
	pthread_mutex_lock(m_server_mutex);
	err = tcp_server_shutdown(m_server);
	pthread_mutex_unlock(m_server_mutex);

	return err == TCP_OK;
}

/**
 * Closes and frees up any resources associated with the current active
 * connection to the server.
 *
 * @return Return value of tcp_server_conn_close.
 *
 * @see gl_server_stop
 * @see tcp_server_conn_close
 */
tcp_err_t gl_server_conn_destroy(void) {
	tcp_err_t err;

	/* Do we even need to do stuff? */
	if (m_conn == NULL)
		return TCP_OK;

	/* Tell the world that we are closing an active connection. */
	if (m_conn->sockfd != -1)
		printf("Closing the currently active connection\n");

	/* Close the connection and free up any resources allocated. */
	err = tcp_server_conn_shutdown(m_conn);
	tcp_server_conn_free(m_conn);
	m_conn = NULL;

	return err;
}

/**
 * Sends an OBEX packet to the client.
 *
 * @param packet OBEX packet object.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_server_send_packet(obex_packet_t *packet) {
	gl_err_t *err;

	pthread_mutex_lock(m_conn_mutex);
	err = obex_net_packet_send(m_conn->sockfd, packet);
	pthread_mutex_unlock(m_conn_mutex);

	return err;
}

/**
 * Handles a client's connection request.
 *
 * @param packet   OBEX packet object.
 * @param accepted Pointer to a boolean that will store a flag of whether the
 *                 connection request was accepted.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_server_handle_conn_req(const obex_packet_t *packet, bool *accepted) {
	gl_err_t *err;
	obex_packet_t *resp;

	/* Trigger the connection requested event. */
	*accepted = true;
	if (evt_server_client_conn_req_cb_func != NULL)
		*accepted = evt_server_client_conn_req_cb_func();

	/* Set our packet length if needed. */
	if (packet->params[2].value.uint16 < m_conn->packet_len)
		m_conn->packet_len = packet->params[2].value.uint16;

	/* Initialize reply packet. */
	if (*accepted) {
		resp = obex_packet_new_success(true, true);
	} else {
		resp = obex_packet_new_unauthorized(true);
	}

	/* Send reply. */
	err = gl_server_send_packet(resp);
	obex_packet_free(resp);

	return err;
}

/**
 * Handles a file transfer from a client.
 *
 * @param init_packet OBEX packet object.
 * @param running     Pointer to a boolean that will store a flag of whether the
 *                    connection should continue running.
 * @param state       Pointer to the variable that's holding the current state
 *                    of the client's connection.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_server_handle_put_req(const obex_packet_t *init_packet, bool *running, conn_state_t *state) {
	gl_err_t *err;
	obex_packet_t *packet;
	obex_packet_t *resp;
	const obex_header_t *header;
	FILE *fh;
	char *fname;
	char *fpath;

	/* Get the file name. */
	header = obex_packet_header_find(init_packet, OBEX_HEADER_NAME);
	if ((header == NULL) || (header->value.wstring.text == NULL)) {
		fname = strdup("Unnamed");
	} else {
		fname = utf16_wcstombs(header->value.wstring.text);
	}

	printf("Receiving a file: %s\n", fname);
	fpath = path_build_download(m_server->download_dir, fname);
	free(fname);
	fname = NULL;
	printf("Saving the file to: %s\n", fpath);

	/* Open the file for download. */
	fh = file_open(fpath, "wb");
	if (fh == NULL) {
		*running = false;
		*state = CONN_STATE_ERROR;

		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN, EMSG("Failed to open "
			"the file for download"));
	}

	/* Write the chunk of data to the file. */
	if (file_write(fh, init_packet->body, init_packet->body_length) < 0) {
		*running = false;
		*state = CONN_STATE_ERROR;

		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN, EMSG("Failed to write "
			"initial data to the downloaded file"));
	}

	/* Initialize reply packet. */
	if (OBEX_IS_FINAL_OPCODE(init_packet->opcode)) {
		resp = obex_packet_new_success(true, false);
	} else {
		resp = obex_packet_new_continue(true);
	}

	/* Send reply. */
	err = gl_server_send_packet(resp);
	obex_packet_free(resp);

	/* Was this transfer just a single packet long? */
	if (OBEX_IS_FINAL_OPCODE(init_packet->opcode)) {
		/* Close the downloaded file handle. */
		file_close(fh);

		/* Trigger the downloaded success event and free the path string. */
		if (evt_server_conn_download_success_cb_func != NULL)
			evt_server_conn_download_success_cb_func(fpath);
		free(fpath);

		return err;
	}

	/* Receive the rest of the file. */
	while ((packet = obex_net_packet_recv(m_conn->sockfd, false)) != NULL) {
		/* Write the chunk of data to the file. */
		if (file_write(fh, packet->body, packet->body_length) < 0) {
			*running = false;
			*state = CONN_STATE_ERROR;
			obex_packet_free(packet);

			return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN, EMSG("Failed to write "
				"chunked data to the downloaded file"));
		}

		/* Initialize reply packet. */
		if (OBEX_IS_FINAL_OPCODE(packet->opcode)) {
			resp = obex_packet_new_success(true, false);
		} else {
			resp = obex_packet_new_continue(true);
		}

		/* Send reply. */
		err = gl_server_send_packet(resp);
		obex_packet_free(resp);

		/* Has the transfer ended? */
		if (OBEX_IS_FINAL_OPCODE(packet->opcode)) {
			/* Free the received packet and close the downloaded file handle. */
			obex_packet_free(packet);
			file_close(fh);

			/* Trigger the downloaded success event and free the path string. */
			if (evt_server_conn_download_success_cb_func != NULL)
				evt_server_conn_download_success_cb_func(fpath);
			free(fpath);

			return err;
		}

		/* Free our resources. */
		obex_packet_free(packet);
	}

	return gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_RECV,
		EMSG("An invalid packet was received during a chunked file transfer"));
	}

/**
 * Waits for the server thread to return.
 *
 * @return TRUE if the operation was successful.
 */
bool gl_server_thread_join(void) {
	gl_err_t *err;
	bool success;

	/* Do we even need to do something? */
	if (m_server_thread == NULL)
		return true;

	/* Join the thread back into us. */
	pthread_join(*m_server_thread, (void **)&err);
	free(m_server_thread);
	m_server_thread = NULL;

	/* Check if we got any returned errors. */
	if (err == NULL)
		return true;

	/* Check for success and free our returned value. */
	success = err->error.generic <= 0;
	gl_error_print(err);
	gl_error_free(err);

	/* Check if the thread join was successful. */
	return success;
}

/**
 * Server thread function.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param args No arguments should be passed.
 *
 * @return Pointer to a gl_err_t with the last error code. This pointer must be
 *         free'd by you.
 */
void *server_thread_func(void *args) {
	tcp_err_t tcp_err;
	gl_err_t *gl_err;

	/* Ignore the argument passed and initialize things. */
	(void)args;
	gl_err = NULL;

	/* Start the server and listen for incoming connections. */
	tcp_err = tcp_server_start(m_server);
	switch (tcp_err) {
		case TCP_OK:
			break;
		case TCP_ERR_ESOCKET:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ESOCKET,
				EMSG("Server failed to create a socket"));
		case TCP_ERR_EBIND:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ECONNECT,
				EMSG("Server failed to bind to a socket"));
		case TCP_ERR_ELISTEN:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ECONNECT,
				EMSG("Server failed to listen on a socket"));
		default:
			return gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
				EMSG("tcp_server_start returned a weird error code"));
	}

	/* Trigger the server started event. */
	if (evt_server_start_cb_func != NULL)
		evt_server_start_cb_func(m_server);

	/* Accept incoming connections. */
	while ((m_conn = tcp_server_conn_accept(m_server, NULL)) != NULL) {
		obex_packet_t *packet;
		conn_state_t state;
		bool running;
		bool has_params;

		/* Check if the connection socket is valid. */
		if (m_conn->sockfd == -1) {
			gl_server_conn_destroy();
			break;
		}

		/* Trigger the connection accepted event. */
		if (evt_server_conn_accept_cb_func != NULL)
			evt_server_conn_accept_cb_func(m_conn);

		/* Read packets until the connection is closed or an error occurs. */
		state = CONN_STATE_CREATED;
		running = true;
		has_params = true;
		while (((packet = obex_net_packet_recv(m_conn->sockfd, has_params)) != NULL) && running) {
			printf("== Packet received ====================\n");
			obex_print_packet(packet);
			printf("\n=======================================\n");

			/* Handle operations for the different states. */
			switch (state) {
				case CONN_STATE_CREATED:
					if (packet->opcode == OBEX_OPCODE_CONNECT) {
						/* Got a connection request packet. */
						gl_err = gl_server_handle_conn_req(packet, &running);
						if (running) {
							state = CONN_STATE_RECV_FILES;
							has_params = false;
						}
					} else {
						/* Invalid opcode for this state. */
						gl_err = gl_error_new(ERR_TYPE_GL,
							GL_ERR_INVALID_STATE_OPCODE,
							EMSG("Invalid opcode for created state"));
						running = false;
					}
					break;
				case CONN_STATE_RECV_FILES:
					/* Receiving files from the client. */
					if (OBEX_IGNORE_FINAL_BIT(packet->opcode) == OBEX_OPCODE_PUT) {
						/* Received a chunk of a file. */
						gl_err = gl_server_handle_put_req(packet, &running, &state);
					} else {
						/* Invalid opcode for this state. */
						gl_err = gl_error_new(ERR_TYPE_GL,
							GL_ERR_INVALID_STATE_OPCODE,
							EMSG("Invalid opcode for file receiving state"));
						running = false;
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
		pthread_mutex_lock(m_conn_mutex);
		gl_server_conn_destroy();
		pthread_mutex_unlock(m_conn_mutex);

		/* Trigger the connection closed event. */
		if (evt_server_conn_close_cb_func != NULL)
			evt_server_conn_close_cb_func();
	}

	/* Stop the server and free up any resources. */
	gl_server_stop();

	/* Trigger the server stopped event. */
	if (evt_server_stop_cb_func != NULL)
		evt_server_stop_cb_func();

	return (void *)gl_err;
}

/**
 * Gets the server object handle.
 *
 * @return Server object handle.
 */
server_t *gl_server_get(void) {
	return m_server;
}

/**
 * Sets the Started event callback function.
 *
 * @param func Started event callback function.
 */
void gl_server_evt_start_set(gl_server_evt_start_func func) {
	evt_server_start_cb_func = func;
}

/**
 * Sets the Connection Accepted event callback function.
 *
 * @param func Connection Accepted event callback function.
 */
void gl_server_conn_evt_accept_set(gl_server_conn_evt_accept_func func) {
	evt_server_conn_accept_cb_func = func;
}

/**
 * Sets the Connection Closed event callback function.
 *
 * @param func Connection Closed event callback function.
 */
void gl_server_conn_evt_close_set(gl_server_conn_evt_close_func func) {
	evt_server_conn_close_cb_func = func;
}

/**
 * Sets the Stopped event callback function.
 *
 * @param func Stopped event callback function.
 */
void gl_server_evt_stop_set(gl_server_evt_stop_func func) {
	evt_server_stop_cb_func = func;
}

/**
 * Sets the Client Connection Requested event callback function.
 *
 * @param func Client Connection Requested event callback function.
 */
void gl_server_evt_client_conn_req_set(gl_server_evt_client_conn_req_func func) {
	evt_server_client_conn_req_cb_func = func;
}

/**
 * Sets the File Downloaded Successfully event callback function.
 *
 * @param func File Downloaded Successfully event callback function.
 */
void gl_server_conn_evt_download_success_set(gl_server_conn_evt_download_success_func func) {
	evt_server_conn_download_success_cb_func = func;
}
