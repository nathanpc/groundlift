/**
 * client.c
 * GroundLift client common components.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdshim.h>
#else
#include <arpa/inet.h>
#endif /* _WIN32 */
#include <stdio.h>
#include <string.h>
#include <utils/logging.h>
#include <utils/threads.h>

#include "defaults.h"
#include "error.h"

/* Private methods. */
gl_err_t *gl_client_send_packet(client_handle_t *handle, obex_packet_t *packet);
gl_err_t *gl_client_send_conn_req(client_handle_t *handle, bool *accepted);
gl_err_t *gl_client_send_put_file(client_handle_t *handle);
void *client_thread_func(void *handle_ptr);
void *peer_discovery_thread_func(void *handle_ptr);

/**
 * Allocates a brand new client handle object and populates it with some sane
 * defaults.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated client handle object or NULL if an error occurred.
 *
 * @see gl_client_setup
 * @see gl_client_free
 */
client_handle_t *gl_client_new(void) {
	client_handle_t *handle;

	/* Allocate some memory for our handle object. */
	handle = (client_handle_t *)malloc(sizeof(client_handle_t));
	if (handle == NULL)
		return NULL;

	/* Set some sane defaults. */
	handle->client = NULL;
	handle->thread = NULL;
	handle->fb = NULL;
	handle->running = false;
	handle->mutexes.client = thread_mutex_new();
	handle->mutexes.send = thread_mutex_new();
	handle->events.connected = NULL;
	handle->events.disconnected = NULL;
	handle->events.request_response = NULL;
	handle->events.put_progress = NULL;
	handle->events.put_succeeded = NULL;

	return handle;
}

/**
 * Sets everything up for the client handle to be usable.
 *
 * @param handle Client's handle object.
 * @param addr   Address to connect to.
 * @param port   Port to connect to.
 * @param fname  Path to a file to be sent to the server.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_connect
 */
gl_err_t *gl_client_setup(client_handle_t *handle, const char *addr,
						  uint16_t port, const char *fname) {
	/* Check if the file exists. */
	if (!file_exists(fname)) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
							EMSG("File to be sent doesn't exist"));
	}

	/* Populate our file bundle. */
	handle->fb = file_bundle_new(fname);
	if (handle->fb == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_FOPEN,
			EMSG("Failed to get information about the file to be transferred"));
	}

	/* Get a client handle. */
	handle->client = tcp_client_new(addr, port);
	if (handle->client == NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_SOCKET,
							EMSG("Failed to initialize the client socket"));
	}

	return NULL;
}

/**
 * Frees up any resources allocated by the client. Shutting down the connection
 * prior to calling this function isn't required.
 *
 * @param handle Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_disconnect
 */
gl_err_t *gl_client_free(client_handle_t *handle) {
	gl_err_t *err;

	/* Do we even have any work to do? */
	if (handle == NULL)
		return NULL;

	/* Free the file bundle. */
	if (handle->fb) {
		file_bundle_free(handle->fb);
		handle->fb = NULL;
	}

	/* Disconnect the client and free up any allocated resources. */
	err = gl_client_disconnect(handle);
	if (err)
		gl_error_print(err);
	tcp_client_free(handle->client);
	handle->client = NULL;

	/* Join the client thread. */
	err = gl_client_thread_join(handle);

	/* Free our client mutexes. */
	thread_mutex_free(&handle->mutexes.send);
	thread_mutex_free(&handle->mutexes.client);

	/* Free ourselves. */
	free(handle);
	handle = NULL;

	return err;
}

/**
 * Connects the client to a server.
 *
 * @param handle Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_loop
 * @see gl_client_disconnect
 */
gl_err_t *gl_client_connect(client_handle_t *handle) {
	thread_err_t ret;

	/* Allocate some memory for our thread. */
	handle->thread = thread_new();
	if (handle->thread == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to allocate the client connection thread"));
	}

	/* Create the client thread. */
	ret = thread_create(handle->thread, client_thread_func, (void *)handle);
	if (ret) {
		handle->thread = NULL;
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to create the client connection thread"));
	}

	return NULL;
}

/**
 * Disconnects the client if needed.
 *
 * @param handle Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_loop
 * @see gl_client_connect
 */
gl_err_t *gl_client_disconnect(client_handle_t *handle) {
	tcp_err_t err;

	/* Do we even need to shut it down? */
	if ((handle == NULL) || (handle->client == NULL))
		return NULL;

	/* Shut the connection down. */
	thread_mutex_lock(handle->mutexes.client);
	err = tcp_client_shutdown(handle->client);
	thread_mutex_unlock(handle->mutexes.client);

	/* Check for socket errors. */
	if (err > SOCK_OK) {
		return gl_error_new(ERR_TYPE_TCP, (int8_t)err,
			EMSG("Failed to properly shutdown the client socket"));
	}

	return NULL;
}

/**
 * Waits for the client thread to return.
 *
 * @param handle Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_thread_join(client_handle_t *handle) {
	gl_err_t *err;

	/* Join the thread back into us. */
	if (thread_join(&handle->thread, (void **)&err) > THREAD_OK) {
		log_errno(LOG_ERROR, "gl_client_thread_join@thread_join");
	}

	return err;
}

/**
 * Allocates a brand new discovery client handle object and populates it with
 * some sane defaults.
 *
 * @warning This function allocates memory that must be free'd by you.
 *
 * @return Newly allocated discovery client handle object or NULL if an error
 *         occurred.
 *
 * @see gl_client_discovery_setup
 * @see gl_client_discovery_free
 */
discovery_client_t *gl_client_discovery_new(void) {
	discovery_client_t *handle;

	/* Allocate some memory for our handle. */
	handle = (discovery_client_t *)malloc(sizeof(discovery_client_t));
	if (handle == NULL)
		return NULL;

	/* Set some sane defaults. */
	handle->sock.sockfd = -1;
	handle->thread = NULL;
	handle->mutexes.client = thread_mutex_new();
	handle->events.discovered_peer = NULL;
	handle->event_args.discovered_peer = NULL;

	return handle;
}

/**
 * Sets up the socket for the peer discovery operation.
 *
 * @param handle Peer discovery client handle object.
 * @param port   UDP port to send to/listen on for discovery packets.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_discover_peers
 */
gl_err_t *gl_client_discovery_setup(discovery_client_t *handle, uint16_t port) {
	tcp_err_t tcp_err;

	/* Sanity check. */
	if (handle == NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_INVALID_HANDLE,
							EMSG("Invalid handle object"));
	}

	/*
	 * TODO: Use INADDR_BROADCAST as fallback, but in modern platforms go
	 *       through network interfaces and broadcast using the subnet mask.
	 *       Create another function to handle this modern scenario and leave
	 *       this function for the fallback.
	 */

	/* Initialize the discovery packet broadcaster. */
	tcp_err = udp_discovery_init(&handle->sock, false, INADDR_BROADCAST, port,
								 UDP_TIMEOUT_MS);
	if (tcp_err) {
		return gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to initialize the client's discovery socket"));
	}

	return NULL;
}

/**
 * Sends a peer discovery broadcast.
 *
 * @param handle Peer discovery client handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 *
 * @see gl_client_discovery_thread_join
 */
gl_err_t *gl_client_discover_peers(discovery_client_t *handle) {
	gl_err_t *err;
	int ret;

	/* Join the previous thread first. */
	err = NULL;
	if (handle->thread != NULL) {
		printf("Previous discovery thread still running. Waiting...\n");
		err = gl_client_discovery_thread_join(handle);
		if (err)
			return err;
	}

	/* Allocate some memory for our new thread. */
	handle->thread = thread_new();
	if (handle->thread == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to allocate the peer discovery thread handle"));
	}

	/* Create a new discovery thread. */
	ret = thread_create(handle->thread, peer_discovery_thread_func,
						(void *)handle);
	if (ret) {
		handle->thread = NULL;
		err = gl_error_new_errno(ERR_TYPE_GL, GL_ERR_THREAD,
			EMSG("Failed to create the client's peer discovery thread"));
	}

	return err;
}

/**
 * Aborts an ongoing peer discovery operation.
 *
 * @param handle Peer discovery client handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_discovery_abort(discovery_client_t *handle) {
	tcp_err_t tcp_err;

	/* Sanity check. */
	if ((handle == NULL) || (handle->sock.sockfd == -1))
		return NULL;

	/* Close the socket. */
	thread_mutex_lock(handle->mutexes.client);
	tcp_err = socket_shutdown(handle->sock.sockfd);
	handle->sock.sockfd = -1;
	thread_mutex_unlock(handle->mutexes.client);

	/* Check for errors. */
	if (tcp_err > SOCK_OK) {
		return gl_error_new_errno(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to close the client peer discovery socket"));
	}

	return NULL;
}

/**
 * Waits for the peer discovery thread to return.
 *
 * @param handle Peer discovery client handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_discovery_thread_join(discovery_client_t *handle) {
	gl_err_t *err;

	/* Join the thread back into us. */
	if (thread_join(&handle->thread, (void **)&err) > THREAD_OK) {
		log_errno(LOG_ERROR, "gl_client_discovery_thread_join@thread_join");
	}

	return err;
}

/**
 * Frees up any resources allocated by the peer discovery service.
 *
 * @param handle Peer discovery client handle object to be free'd.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_discovery_free(discovery_client_t *handle) {
	gl_err_t *err;

	/* Do we even have to do anything? */
	if (handle == NULL)
		return NULL;

	/* Close the connection if needed. */
	err = gl_client_discovery_abort(handle);
	if (err)
		gl_error_print(err);

	/* Join the thread if needed. */
	err = gl_client_discovery_thread_join(handle);

	/* Free our client mutex. */
	thread_mutex_free(&handle->mutexes.client);

	/* Free ourselves. */
	free(handle);
	handle = NULL;

	return err;
}

/**
 * Sends an OBEX packet to the server.
 *
 * @param handle Client's handle object.
 * @param packet OBEX packet object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_send_packet(client_handle_t *handle, obex_packet_t *packet) {
	gl_err_t *err;

	thread_mutex_lock(handle->mutexes.client);
	thread_mutex_lock(handle->mutexes.send);
	err = obex_net_packet_send(handle->client->sockfd, packet);
	thread_mutex_unlock(handle->mutexes.send);
	thread_mutex_unlock(handle->mutexes.client);

	return err;
}

/**
 * Handles a connection request.
 *
 * @param handle   Client's handle object.
 * @param accepted Pointer to a boolean that will store a flag of whether the
 *                 connection request was accepted.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_send_conn_req(client_handle_t *handle, bool *accepted) {
	gl_err_t *err;
	obex_packet_t *packet;

	/* Initialize variables. */
	err = NULL;

	/* TODO: Properly handle files with a size larger than 32-bits. */
	/* Create the OBEX packet. */
	packet = obex_packet_new_connect(handle->fb->base, (uint32_t *)&handle->fb->size);

	/* Send the packet. */
	err = gl_client_send_packet(handle, packet);
	if (err)
		goto cleanup;

	/* Read the response packet. */
	obex_packet_free(packet);
	thread_mutex_lock(handle->mutexes.client);
	packet = obex_net_packet_recv(handle->client->sockfd, NULL, true);
	thread_mutex_unlock(handle->mutexes.client);
	if ((packet == NULL) || (packet == obex_invalid_packet)) {
		err = gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_RECV,
			EMSG("Failed to receive or decode the received packet"));
		goto cleanup;
	}

	/* Check if our request was accepted. */
	*accepted = packet->opcode == OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS);

	/* Set our packet length if needed. */
	if (packet->params[2].value.uint16 < handle->client->packet_len)
		handle->client->packet_len = packet->params[2].value.uint16;

cleanup:
	/* Free up any resources. */
	obex_packet_free(packet);

	return err;
}

/**
 * Handles the send operation of a file.
 *
 * @param handle Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
gl_err_t *gl_client_send_put_file(client_handle_t *handle) {
	gl_err_t *err;
	gl_client_progress_t progress;
	FILE *fh;
	uint16_t csize;
	uint32_t chunks;
	uint32_t cc;

	/* Set some defaults. */
	err = NULL;

	/* Calculate the size of each file chunk. */
	csize = OBEX_MAX_FILE_CHUNK;
	if (handle->client->packet_len < OBEX_MAX_FILE_CHUNK)
		csize = handle->client->packet_len; /* TODO: Properly take into account the negotiated packet size. */
	chunks = (uint32_t)(handle->fb->size / csize);
	if ((csize * chunks) < handle->fb->size)
		chunks++;

#ifdef DEBUG
	printf("file size %ld csize %u chunks %u\n", handle->fsize, csize, chunks);
#endif

	/* Open the file. */
	fh = file_open(handle->fb->name, "rb");
	if (fh == NULL) {
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FOPEN,
							EMSG("Failed to open the file for upload"));
	}

	/* Get the basename of the file and populate the information structure. */
	progress.fb = handle->fb;
	progress.csize = csize;
	progress.chunks = chunks;
	progress.sent_chunk = 0;
	progress.sent_bytes = 0;

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
			err = gl_error_new(ERR_TYPE_GL, GL_ERR_FREAD,
				EMSG("Failed to read a part of the file for upload"));
			break;
		}

		/* Create the OBEX packet object. */
		if (cc == 0) {
			packet = obex_packet_new_put(handle->fb->base,
										 (uint32_t *)&handle->fb->size,
										 last_chunk);
		} else {
			packet = obex_packet_new_put(handle->fb->base, NULL, last_chunk);
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
		err = obex_net_packet_send(handle->client->sockfd, packet);
		progress.sent_bytes += (uint32_t)len;
		obex_packet_free(packet);
		if (err)
			break;

		/* Update the progress of the transfer via an event. */
		if (handle->events.put_progress != NULL) {
			progress.sent_chunk = cc + 1;
			handle->events.put_progress(&progress);
		}

		/* Read the response packet. */
		thread_mutex_lock(handle->mutexes.client);
		packet = obex_net_packet_recv(handle->client->sockfd, NULL, false);
		thread_mutex_unlock(handle->mutexes.client);
		if ((packet == NULL) || (packet == obex_invalid_packet)) {
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
		return gl_error_new(ERR_TYPE_GL, GL_ERR_FCLOSE,
							EMSG("Failed to close the file that was uploaded"));
	}

	/* Trigger the put succeeded event. */
	if ((err == NULL) && (handle->events.put_succeeded != NULL))
		handle->events.put_succeeded(handle->fb);

	return err;
}

/**
 * Client's thread function.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param handle_ptr Client's handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
void *client_thread_func(void *handle_ptr) {
	client_handle_t *handle;
	tcp_err_t tcp_err;
	gl_err_t *gl_err;

	/* Ignore the argument passed. */
	gl_err = NULL;
	handle = (client_handle_t *)handle_ptr;
	handle->running = false;

	/* Sanity check. */
	if (handle->client == NULL) {
		return gl_error_new(ERR_TYPE_TCP, TCP_ERR_UNKNOWN,
							EMSG("Client socket isn't setup"));
	}

	/* Connect our client to a server. */
	tcp_err = tcp_client_connect(handle->client);
	switch (tcp_err) {
		case SOCK_OK:
			break;
		case SOCK_ERR_ESOCKET:
			return gl_error_new(ERR_TYPE_TCP, SOCK_ERR_ESOCKET,
				EMSG("Client failed to create a socket"));
		case TCP_ERR_ECONNECT:
			return gl_error_new(ERR_TYPE_TCP, TCP_ERR_ECONNECT,
				EMSG("Client failed to connect to server"));
		default:
			return gl_error_new(ERR_TYPE_TCP, (int8_t)tcp_err,
				EMSG("tcp_client_connect returned a weird error code"));
	}

	/* Trigger the connected event callback. */
	if (handle->events.connected != NULL)
		handle->events.connected(handle->client);

	/* Send the OBEX connection request and trigger an event upon reply. */
	gl_err = gl_client_send_conn_req(handle, &handle->running);
	if (handle->events.request_response != NULL)
		handle->events.request_response(handle->fb, handle->running);
	if (!handle->running || (gl_err != NULL))
		goto disconnect;

	/* Send the file to the server. */
	gl_err = gl_client_send_put_file(handle);
	if (!handle->running || (gl_err != NULL))
		goto disconnect;

disconnect:
	/* Disconnect from server, free up any resources, and trigger callback. */
	gl_client_disconnect(handle);
	if (handle->events.disconnected != NULL)
		handle->events.disconnected(handle->client);

	return (void *)gl_err;
}

/**
 * Thread function that handles everything related to peer discovery.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param handle_ptr Peer discovery client handle object.
 *
 * @return Error information or NULL if the operation was successful. This
 *         pointer must be free'd by you.
 */
void *peer_discovery_thread_func(void *handle_ptr) {
	discovery_client_t *handle;
	obex_packet_t *packet;
	obex_opcodes_t op;
	gl_err_t *err;

	/* Initialize some variables. */
	handle = (discovery_client_t *)handle_ptr;
	err = NULL;

	/* Build up the discovery packet. */
	packet = obex_packet_new_get("DISCOVER", true);
	if (packet == NULL) {
		return gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_PACKET_NEW,
			EMSG("Failed to create the discovery OBEX packet"));
	}

	/* Send discovery broadcast. */
	err = obex_net_packet_sendto(handle->sock, packet);
	obex_packet_free(packet);
	if (err)
		return (void *)err;

	/* Listen for replies. */
	op = OBEX_SET_FINAL_BIT(OBEX_RESPONSE_SUCCESS);
	while ((packet = obex_net_packet_recvfrom(&handle->sock, &op, false))
		   != NULL) {
		const obex_header_t *header;

		/* Check if we got an invalid packet. */
		if (packet == obex_invalid_packet)
			continue;

		/* Get the peer's hostname. */
		header = obex_packet_header_find(packet, OBEX_HEADER_EXT_HOSTNAME);
		if (header == NULL) {
			/* Free up resources. */
			obex_packet_free(packet);
			thread_mutex_lock(handle->mutexes.client);
			socket_close(handle->sock.sockfd);
			handle->sock.sockfd = -1;
			thread_mutex_unlock(handle->mutexes.client);

			return gl_error_new(ERR_TYPE_OBEX, OBEX_ERR_HEADER_NOT_FOUND,
				EMSG("Discovered peer replied without a hostname"));
		}

		/* Trigger the discovered peer event handler. */
		if (handle->events.discovered_peer) {
			gl_discovery_peer_t *peer;

			/* Allocate some memory for the peer object. */
			peer = (gl_discovery_peer_t *)malloc(sizeof(gl_discovery_peer_t));
			if (peer == NULL) {
				return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
					EMSG("Failed to allocate the discovered peer object"));
			}

			/* Populate the discovered peer object. */
			peer->name = header->value.string.text;
			peer->sock = &handle->sock;
			
			/* Call the event handler and free the peer object. */
			handle->events.discovered_peer(peer,
										   handle->event_args.discovered_peer);
			free(peer);
		}

		/* Free up resources. */
		obex_packet_free(packet);
	}

	/* Close our UDP socket. */
	err = gl_client_discovery_abort(handle);

	return (void *)err;
}

/**
 * Sets the Connected event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Connected event callback function.
 */
void gl_client_evt_conn_set(client_handle_t *handle,
							gl_client_evt_conn_func func) {
	handle->events.connected = func;
}

/**
 * Sets the Disconnected event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Disconnected event callback function.
 */
void gl_client_evt_disconn_set(client_handle_t *handle,
							   gl_client_evt_disconn_func func) {
	handle->events.disconnected = func;
}

/**
 * Sets the Connection Request Reply event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Connection Request Reply event callback function.
 */
void gl_client_evt_conn_req_resp_set(client_handle_t *handle,
									 gl_client_evt_conn_req_resp_func func) {
	handle->events.request_response = func;
}

/**
 * Sets the Send File Operation Progress event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Send File Operation Succeeded event callback function.
 */
void gl_client_evt_put_progress_set(client_handle_t *handle,
									gl_client_evt_put_progress_func func) {
	handle->events.put_progress = func;
}

/**
 * Sets the Send File Operation Succeeded event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Send File Operation Succeeded event callback function.
 */
void gl_client_evt_put_succeed_set(client_handle_t *handle,
								   gl_client_evt_put_succeed_func func) {
	handle->events.put_succeeded = func;
}

/**
 * Sets the Send File Operation Succeeded event callback function.
 *
 * @param handle Client's handle object.
 * @param func   Send File Operation Succeeded event callback function.
 * @param arg    Optional parameter to be sent to the event handler.
 */
void gl_client_evt_discovery_peer_set(discovery_client_t *handle,
									  gl_client_evt_discovery_peer_func func,
									  void *arg) {
	handle->events.discovered_peer = func;
	handle->event_args.discovered_peer = arg;
}
