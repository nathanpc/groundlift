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
		return gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to properly shutdown the client socket"));
	}

	return NULL;
}

/**
 * Discovers the peers on the network from all the possible interfaces (if
 * SINGLE_IFACE_MODE is not defined) and returns a list of what was found.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param peers List of peers that were found on the network.
 *
 * @return Error report or NULL if the operation was successful.
 */
gl_err_t *gl_client_discover_peers(gl_peer_list_t **peers) {
#ifndef SINGLE_IFACE_MODE
	iface_info_list_t *if_list;
	gl_err_t *err;
	uint8_t i;

	/* Initialize some variables. */
	err = NULL;

	/* Get the list of network interfaces. */
	err = socket_iface_info_list(&if_list);
	if (err) {
		return gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENT,
		                     EMSG("Failed to get list of network interfaces"));
	}

	/* TODO: Determine what to do about peers. */

	/* Go through the network interfaces. */
	for (i = 0; i < if_list->count; i++) {
		iface_info_t *iface;

		/* Get the network interface and print its name. */
		iface = if_list->ifaces[i];
#ifdef DEBUG
		log_printf(LOG_INFO, EMSG("Searching for peers on %s...\n"),
		           iface->name);
#endif /* DEBUG */

		/* Search for peers on the network. */
		if (iface->brdaddr->sa_family == AF_INET) {
			err = gl_client_discover_peers_inaddr(peers,
				((struct sockaddr_in *)iface->brdaddr)->sin_addr.s_addr);
			if (err)
				goto cleanup;
		} else {
#ifdef DEBUG
			log_msg(LOG_WARNING, EMSG("Got an IPv6 address for broadcasting "
			                          "the discovery message"));
#endif /* DEBUG */
		}

		if (i < (if_list->count - 1))
			printf("\n");
	}

cleanup:
	/* Free our network interface list. */
	socket_iface_info_list_free(if_list);

	return err;
#else
	return gl_client_discover_peers_inaddr(peers, INADDR_ANY);
#endif /* !SINGLE_IFACE_MODE */
}

/**
 * Discovers the peers on the network and returns a list of what we've found.
 *
 * @warning This function allocates memory that must be freed by you.
 *
 * @param peers  List of peers that were found on the network.
 * @param inaddr Address to send the broadcast message to.
 *
 * @return Error report or NULL if the operation was successful.
 */
gl_err_t *gl_client_discover_peers_inaddr(gl_peer_list_t **peers,
                                          in_addr_t inaddr) {
	glproto_type_t type;
	glproto_msg_t *msg;
	sock_handle_t *client;
	gl_err_t *err;
	sock_err_t serr;

	/* TODO: Append to peers if not NULL. */

	/* Create the client handle. */
	client_handle_t *handle = gl_client_new();
	if (handle == NULL) {
		return gl_error_push(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to allocate a client for peer discovery"));
	}

	/* Get a socket handle. */
	msg = NULL;
	client = NULL;
	handle->sock = socket_new();
	if (handle->sock == NULL) {
		err = gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to allocate the peer discovery socket"));
		goto cleanup;
	}

	/* Set up the socket for broadcasting. */
	socket_setaddr_inaddr(handle->sock, inaddr, GL_SERVER_MAIN_PORT);
	err = socket_setup_udp(handle->sock, false, 1000);
	if (err) {
		err = gl_error_push_errno(ERR_TYPE_GL, GL_ERR_CLIENT,
			EMSG("Failed to set up the peer discovery socket"));
		goto cleanup;
	}

	/* Build up the discovery broadcast message and send it! */
	msg = glproto_msg_new_our(GLPROTO_TYPE_DISCOVERY);
	err = glproto_msg_sendto(handle->sock, msg);
	glproto_msg_free(msg);
	msg = NULL;
	if (err)
		goto cleanup;

	/* Listen for discovery reply messages. */
	while ((err = glproto_recvfrom(handle->sock, &type, &msg, &client, &serr))
			== NULL) {
		/* Check if the discovery period has ended. */
		if (serr == SOCK_EVT_TIMEOUT)
			break;

		/* Check if the message should be ignored. */
		if (type != GLPROTO_TYPE_DISCOVERY) {
			gl_error_push(ERR_TYPE_GL, GL_ERR_WARNING,
				EMSG("Got a discovery reply that wasn't a discovery message"));
			continue;
		}

		/* Free up the allocated message. */
		glproto_msg_free(msg);
		msg = NULL;
		socket_free(client);
		client = NULL;
	}

cleanup:
	/* Free up any allocated resources. */
	glproto_msg_free(msg);
	msg = NULL;
	socket_free(client);
	client = NULL;
	gl_client_free(handle);
	handle = NULL;

	/* Ensure we don't miss any errors. */
	if (err == NULL)
		return gl_error_last();

	return err;
}
