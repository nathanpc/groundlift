/**
 * client.c
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <pthread.h>

#include "defaults.h"
#include "error.h"
#include "obex.h"
#include "fileutils.h"

/* Private variables. */
static tcp_client_t *m_client;
static pthread_mutex_t *m_client_mutex;
static pthread_mutex_t *m_client_send_mutex;
static pthread_t *m_client_thread;

/* Function callbacks. */
static gl_client_evt_conn_func evt_conn_cb_func;
static gl_client_evt_close_func evt_close_cb_func;
static gl_client_evt_disconn_func evt_disconn_cb_func;
static gl_client_evt_conn_req_accepted_func evt_conn_req_accepted_cb_func;
static gl_client_evt_put_succeed_func evt_put_succeed_cb_func;

/* Private methods. */
void *client_thread_func(void *fname);

/**
 * Initializes everything related to the client to a known clean state.
 *
 * @param addr Address to connect to.
 * @param port Port to connect to.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_connect
 */
bool gl_client_init(const char *addr, uint16_t port) {
	/* Ensure we have everything in a known clean state. */
	m_client_thread = (pthread_t *)malloc(sizeof(pthread_t));
	evt_conn_cb_func = NULL;
	evt_close_cb_func = NULL;
	evt_disconn_cb_func = NULL;
	evt_conn_req_accepted_cb_func = NULL;
	evt_put_succeed_cb_func = NULL;

	/* Initialize our mutexes. */
	m_client_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_client_mutex, NULL);
	m_client_send_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(m_client_send_mutex, NULL);

	/* Get a server handle. */
	m_client = tcp_client_new(addr, port);
	return m_client != NULL;
}

/**
 * Frees up any resources allocated by the client. Shutting down the connection
 * prior to calling this function isn't required.
 *
 * @see gl_client_disconnect
 */
void gl_client_free(void) {
	/* Disconnect the client and free up any allocated resources. */
	gl_client_disconnect();
	tcp_client_free(m_client);
	m_client = NULL;

	/* Join the client thread. */
	gl_client_thread_join();

	/* Free our client send mutex. */
	if (m_client_send_mutex) {
		free(m_client_send_mutex);
		m_client_send_mutex = NULL;
	}

	/* Free our client mutex. */
	if (m_client_mutex) {
		free(m_client_mutex);
		m_client_mutex = NULL;
	}
}

/**
 * Connects the client to a server.
 *
 * @param fname Path to a file to be sent to the server.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_loop
 * @see gl_client_disconnect
 */
bool gl_client_connect(char *fname) {
	int ret;

	/* Create the client thread. */
	ret = pthread_create(m_client_thread, NULL, client_thread_func,
		(void *)fname);
	if (ret)
		m_client_thread = NULL;

	return ret == 0;
}

/**
 * Disconnects the client if needed.
 *
 * @return TRUE if the operation was successful.
 *
 * @see gl_client_loop
 * @see gl_client_connect
 */
bool gl_client_disconnect(void) {
	tcp_err_t err;

	/* Do we even need to shut it down? */
	if (m_client == NULL)
		return true;

	/* Shut the connection down. */
	pthread_mutex_lock(m_client_mutex);
	err = tcp_client_shutdown(m_client);
	pthread_mutex_unlock(m_client_mutex);

	return err == TCP_OK;
}

/**
 * Sends an OBEX packet to the server.
 *
 * @param packet OBEX packet object.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_client_send_packet(obex_packet_t *packet) {
	gl_err_t *err;

	pthread_mutex_lock(m_client_mutex);
	pthread_mutex_lock(m_client_send_mutex);
	err = obex_net_packet_send(m_client->sockfd, packet);
	pthread_mutex_unlock(m_client_send_mutex);
	pthread_mutex_unlock(m_client_mutex);

	return err;
}

/**
 * Handles a connection request.
 *
 * @param accepted Pointer to a boolean that will store a flag of whether the
 *                 connection request was accepted.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_client_send_conn_req(bool *accepted) {
	gl_err_t *err;
	obex_packet_t *packet;

	/* Initialize variables. */
	err = NULL;

	/* Create the OBEX packet. */
	packet = obex_packet_new_connect();

	/* Send the packet. */
	err = gl_client_send_packet(packet);
	if (err)
		goto cleanup;

	/* Read the response packet. */
	obex_packet_free(packet);
	pthread_mutex_lock(m_client_mutex);
	packet = obex_net_packet_recv(m_client->sockfd, true);
	pthread_mutex_unlock(m_client_mutex);
	if (packet == NULL) {
		err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_RECV,
			EMSG("Failed to receive or decode the received packet"));
		goto cleanup;
	}

	printf("== Packet received ====================\n");
	obex_print_packet(packet);
	printf("\n=======================================\n");

	/* Check if our request was accepted. */
	*accepted = packet->opcode == OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS);

cleanup:
	/* Free up any resources. */
	obex_packet_free(packet);

	return err;
}

/**
 * Handles the send operation of a file.
 *
 * @param fname Path to a file to be sent.
 * @param psize Maximum size of each packet to be sent.
 *
 * @return An error object if an error occurred or NULL if it was successful.
 */
gl_err_t *gl_client_send_put_file(const char *fname, uint16_t psize) {
	gl_err_t *err;
	FILE *fh;
	int64_t fsize;
	uint16_t csize;
	uint8_t chunks;
	uint8_t cc;

	/* Set some defaults. */
	err = NULL;

	/* Get the size of the full file for transferring. */
	fsize = file_size(fname);
	if (fsize < 0L) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FSIZE, EMSG("Failed to get the "
			"length of the file for upload"));
	}

	/* Calculate the size of each file chunk. */
	csize = OBEX_MAX_FILE_CHUNK;
	if (psize < OBEX_MAX_FILE_CHUNK)
		csize = psize;  /* TODO: Properly take into account the negotiated packet size. */
	chunks = (uint8_t)(fsize / csize);
	if ((csize * chunks) < fsize)
		chunks++;

	printf("file size %ld csize %u chunks %u\n", fsize, csize, chunks);

	/* Open the file. */
	fh = file_open(fname, "rb");
	if (fh == NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN, EMSG("Failed to open "
			"the file for upload"));
	}

	/* Read the file in chunks for sending over OBEX packets. */
	for (cc = 0; cc < chunks; cc++) {
		obex_packet_t *packet;
		void *buf;
		ssize_t len;
		bool last_chunk;

		/* Are we in our last chunk? */
		last_chunk = cc == (chunks - 1);

		/* Read a chunk of the file. */
		len = file_read(fh, &buf, csize);
		if (len < 0L) {
			err = gl_error_new(ERR_TYPE_GL, GL_ERR_FREAD, EMSG("Failed to read "
				"a part of the file for upload"));
			break;
		}

		/* Create the OBEX packet object. */
		if (cc == 0) {
			/* TODO: Get only the basename+ext of the file for PUT. */
			packet = obex_packet_new_put(fname, (uint32_t *)&fsize, last_chunk);
		} else {
			packet = obex_packet_new_put(NULL, NULL, last_chunk);
		}

		/* Check if the packet object was created. */
		if (packet == NULL) {
			err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_NEW,
				EMSG("Failed to create the PUT packet for file upload"));
			free(buf);

			break;
		}

		/* Put the chunk of the file in the body of the packet. */
		obex_packet_body_set(packet, (uint16_t)len, buf, last_chunk);

		/* Send the packet. */
		err = obex_net_packet_send(m_client->sockfd, packet);
		obex_packet_free(packet);
		if (err)
			break;

		/* Read the response packet. */
		pthread_mutex_lock(m_client_mutex);
		packet = obex_net_packet_recv(m_client->sockfd, false);
		pthread_mutex_unlock(m_client_mutex);
		if (packet == NULL) {
			err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_RECV,
				EMSG("Failed to receive or decode the received packet"));
			break;
		}

		/* Check if the response was correct. */
		switch (packet->opcode) {
			case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS):
				if (!last_chunk) {
					err = gl_error_new(ERR_TYPE_GL, GL_ERR_INVALID_STATE_OPCODE,
						EMSG("SUCCESS packet received before last file chunk"));
				}
				break;
			case OBEX_SET_FINAL_BIT(OBEX_RESPONSE_CONTINUE):
				if (last_chunk) {
					err = gl_error_new(ERR_TYPE_GL, GL_ERR_INVALID_STATE_OPCODE,
						EMSG("CONTINUE packet received for last file chunk"));
				}
				break;
			default:
				err = gl_error_new(ERR_TYPE_GL, GL_ERR_INVALID_STATE_OPCODE,
					EMSG("Invalid opcode response for PUT operation"));
				break;
		}

		/* Free our response packet and check for errors. */
		obex_packet_free(packet);
		if (err)
			break;
	}

	/* Close the file. */
	if (!file_close(fh)) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FCLOSE, EMSG("Failed to close "
			"the file that was uploaded"));
	}

	/* Trigger the put succeeded event. */
	if (evt_put_succeed_cb_func != NULL)
		evt_put_succeed_cb_func(fname);

	return err;
}

/**
 * Waits for the client thread to return.
 *
 * @return TRUE if the operation was successful.
 */
bool gl_client_thread_join(void) {
	gl_err_t *err;
	bool success;

	/* Do we even need to do something? */
	if (m_client_thread == NULL)
		return true;

	/* Join the thread back into us. */
	pthread_join(*m_client_thread, (void **)&err);
	free(m_client_thread);
	m_client_thread = NULL;

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
 * Client's thread function.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param fname Path to a file to be sent.
 *
 * @return Pointer to a gl_err_t with the last error code. This pointer must be
 *         free'd by you.
 */
void *client_thread_func(void *fname) {
	tcp_err_t tcp_err;
	gl_err_t *gl_err;
	bool running;

	/* Ignore the argument passed. */
	gl_err = NULL;
	running = false;

	/* Check if the file exists. */
	if (!file_exists(fname)) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
							EMSG("File to be sent doesn't exist"));
	}

	/* Get a client handle. */
	if (m_client == NULL) {
		return gl_error_new(ERR_TYPE_TCP, TCP_ERR_UNKNOWN,
							EMSG("Client handle is NULL"));
	}

	/* Connect our client to a server. */
	tcp_err = tcp_client_connect(m_client, NULL);
	switch (tcp_err) {
		case TCP_OK:
			break;
		case TCP_ERR_ESOCKET:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ESOCKET,
				EMSG("Client failed to create a socket"));
		case TCP_ERR_ECONNECT:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ECONNECT,
				EMSG("Client failed to connect to server"));
		default:
			return gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
				EMSG("tcp_client_connect returned a weird error code"));
	}

	/* Trigger the connected event callback. */
	if (evt_conn_cb_func != NULL)
		evt_conn_cb_func(m_client);

	/* Send the OBEX connection request. */
	gl_err = gl_client_send_conn_req(&running);
	if (!running || (gl_err != NULL))
		goto disconnect;

	/* Trigger the connection accepted event callback. */
	if (evt_conn_req_accepted_cb_func != NULL)
		evt_conn_req_accepted_cb_func();

	/* Send the file to the server. */
	gl_err = gl_client_send_put_file((const char *)fname, 10);
	if (!running || (gl_err != NULL))
		goto disconnect;

disconnect:
	/* Disconnect from server, free up any resources, and trigger callback. */
	gl_client_disconnect();
	if (evt_disconn_cb_func != NULL)
		evt_disconn_cb_func(m_client);

	return (void *)gl_err;
}

/**
 * Gets the client object handle.
 *
 * @return Client object handle.
 */
tcp_client_t *gl_client_get(void) {
	return m_client;
}

/**
 * Sets the Connected event callback function.
 *
 * @param func Connected event callback function.
 */
void gl_client_evt_conn_set(gl_client_evt_conn_func func) {
	evt_conn_cb_func = func;
}

/**
 * Sets the Connection Closed event callback function.
 *
 * @param func Connection Closed event callback function.
 */
void gl_client_evt_close_set(gl_client_evt_close_func func) {
	evt_close_cb_func = func;
}

/**
 * Sets the Disconnected event callback function.
 *
 * @param func Disconnected event callback function.
 */
void gl_client_evt_disconn_set(gl_client_evt_disconn_func func) {
	evt_disconn_cb_func = func;
}

/**
 * Sets the Connection Request Accepted event callback function.
 *
 * @param func Connection Request Accepted event callback function.
 */
void gl_client_evt_conn_req_accepted_set(gl_client_evt_conn_req_accepted_func func) {
	evt_conn_req_accepted_cb_func = func;
}

/**
 * Sets the Send File Operation Succeeded event callback function.
 *
 * @param func Send File Operation Succeeded event callback function.
 */
void gl_client_evt_put_succeed_set(gl_client_evt_put_succeed_func func) {
	evt_put_succeed_cb_func = func;
}
