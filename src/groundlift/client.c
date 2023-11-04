/**
 * client.c
 * GroundLift client request making components.
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

/**
 * Allocates a brand new client handle object.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @return Newly allocated client handle object or NULL if an error occurred.
 *
 * @see gl_client_connect
 * @see gl_client_free
 */
client_handle_t *gl_client_new(void) {
	client_handle_t *handle;

	/* Allocate some memory for our handle object. */
	handle = (client_handle_t *)malloc(sizeof(client_handle_t));
	if (handle == NULL)
		return NULL;

	/* Set some sane defaults. */
	handle->sock = NULL;
	handle->fb = NULL;
	handle->running = false;

	return handle;
}

/**
 * Frees up any resources allocated by the client. Shutting down the connection
 * prior to calling this function isn't required.
 *
 * @param handle Client's handle object.
 *
 * @return Error report or NULL if the operation was successful.
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
	if (err) {
		err = gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENT,
							EMSG("Failed to disconnect the client on free"));
	}
	socket_free(handle->sock);
	handle->sock = NULL;

	/* Free ourselves. */
	free(handle);
	handle = NULL;

	return err;
}

/**
 * Connects the client to a server.
 *
 * @param handle Client's handle object.
 * @param addr   Address to connect to.
 * @param port   Port to connect to.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_client_disconnect
 */
gl_err_t *gl_client_connect(client_handle_t *handle, const char *addr,
							uint16_t port) {
	gl_err_t *err;

    /* Get a socket handle. */
    handle->sock = socket_new();
    if (handle->sock == NULL) {
        return gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to allocate the client socket"));
    }

    /* Set up the socket. */
    socket_setaddr(handle->sock, addr, port);
    err = socket_setup_udp(handle->sock, false, 1000);
    if (err) {
        return gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
								   EMSG("Failed to set up the client socket"));
    }

	return NULL;
}

/**
 * Disconnects the client if needed.
 *
 * @param handle Client's handle object.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_client_connect
 */
gl_err_t *gl_client_disconnect(client_handle_t *handle) {
	gl_err_t *err;

	/* Do we even need to shut it down? */
	if ((handle == NULL) || (handle->sock == NULL))
		return NULL;

	/* Shut the connection down. */
	err = socket_shutdown(handle->sock);
	if (err) {
		return gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENt,
			EMSG("Failed to properly shutdown the client socket"));
	}

	return NULL;
}

/**
 * Discovers the peers on the network and returns a list of what we've found.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param peers List of peers that were found on the network.
 *
 * @return Error report or NULL if the operation was successful.
 */
gl_err_t *gl_client_discover_peers(gl_peer_list_t *peers) {
	glproto_type_t type;
	glproto_msg_t *msg;
	gl_err_t *err;

	/* Create the client handle. */
	client_handle_t *handle = gl_client_new();
	if (handle == NULL) {
		return gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to allocate a client for peer discovery"));
	}

	/* Get a socket handle. */
	handle->sock = socket_new();
	if (handle->sock == NULL) {
		err = gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to allocate the peer discovery socket"));
		goto cleanup;
	}

	/* Set up the socket for broadcasting. */
	/* TODO: Should we pass as a server? */
	socket_setaddr(handle->sock, NULL, GL_SERVER_MAIN_PORT);
	err = socket_setup_udp(handle->sock, false, 1000);
	if (err) {
		err = gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to set up the peer discovery socket"));
		goto cleanup;
	}

	/* Build up the discovery broadcast message and send it! */
	msg = NULL;
	msg = glproto_msg_new_our(msg, GLPROTO_TYPE_DISCOVERY);
	err = glproto_msg_sendto(handle->sock, msg);
	glproto_msg_free(msg);
	if (err)
		goto cleanup;

	/* Listen for discovery messages. */
	msg = NULL;
	while ((err = glproto_recvfrom(handle->sock, &type, &msg)) == NULL) {
		/* Check if the message should be ignored. */
		if (type != GLPROTO_TYPE_DISCOVERY)
			continue;

		glproto_msg_print(msg);
	}

cleanup:
	/* Free up any allocated resources. */
	glproto_msg_free(msg);
	msg = NULL;
	gl_client_free(handle);
	handle = NULL;

	/* Ensure we don't miss any errors. */
	if (err == NULL)
		return gl_error_last();

	return err;
}

#if 0
/**
 * Sends a file to another peer.
 *
 * @param handle Client's handle object.
 * @param fname  Path to a file to be sent to the server.
 *
 * @return Error report or NULL if the operation was successful.
 *
 * @see gl_client_connect
 */
gl_err_t *gl_client_send_file(client_handle_t *handle, const char *fname) {
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

	return NULL;
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
		/* TODO: Properly take into account the negotiated packet size. */
		csize = handle->client->packet_len;
	chunks = (uint32_t)(handle->fb->size / csize);
	if ((csize * chunks) < handle->fb->size)
		chunks++;

#ifdef DEBUG
	printf("file size %ld csize %u chunks %u\n", handle->fb->size, csize,
		   chunks);
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
		if (handle->events.send_progress != NULL) {
			progress.sent_chunk = cc + 1;
			handle->events.send_progress(&progress,
										handle->event_args.send_progress);
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
	if ((err == NULL) && (handle->events.send_succeeded != NULL))
		handle->events.send_succeeded(handle->fb,
									 handle->event_args.send_succeeded);

	return err;
}
#endif

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

	/* Trigger the discovery finished event handler. */
	if (handle->events.finished)
		handle->events.finished(handle->event_args.finished);

	return (void *)err;
}

/**
 * Creates a full copy of a peer object.
 * @warning This function allocates memory that must be free'd by you.
 *
 * @param peer Peer object to be duplicated.
 *
 * @return Duplicate peer object or NULL if an error occurred.
 *
 * @see gl_peer_free
 */
gl_peer_t *gl_peer_dup(const gl_discovery_peer_t *peer) {
	gl_peer_t *dup;

	/* Allocate some memory for the duplicate peer object. */
	dup = (gl_peer_t *)malloc(sizeof(gl_peer_t));
	if (dup == NULL) {
		log_errno(LOG_ERROR,
				  EMSG("Failed to allocate the discovered peer object"));
		return NULL;
	}

	/* Populate the duplicate object. */
	dup->name = strdup(peer->name);
	dup->sock = socket_bundle_dup(peer->sock);

	return dup;
}

/**
 * Frees any resources allocated for a peer object, including itself.
 *
 * @param peer Peer object to be free'd.
 */
void gl_peer_free(gl_peer_t *peer) {
	/* Free the object's properties. */
	free(peer->name);
	socket_bundle_free(peer->sock);

	/* Free the peer object itself. */
	free(peer);
}
