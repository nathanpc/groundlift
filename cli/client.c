/**
 * client.c
 * GroundLift command-line client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "client.h"

#include <groundlift/client.h>
#include <groundlift/defaults.h>
#include <groundlift/error.h>
#include <utils/logging.h>

/* Public variables. */
client_handle_t *g_client;

#if 0
/**
 * Perform an entire send exchange with the server.
 *
 * @param ip    Server's IP to send the data to.
 * @param port  Server port to talk to.
 * @param fname Path to a file to be sent to the server.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_send(const char *ip, uint16_t port, const char *fname) {
	gl_err_t *err = NULL;

	/* Construct the client handle object. */
	g_client = gl_client_new();
	if (g_client == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the client handle object"));
	}

	/* Setup event handlers. */
	gl_client_evt_conn_set(g_client, event_connected, NULL);
	gl_client_evt_conn_req_resp_set(g_client, event_conn_req_resp, NULL);
	gl_client_evt_put_progress_set(g_client, event_send_progress, NULL);
	gl_client_evt_put_succeed_set(g_client, event_send_success, NULL);
	gl_client_evt_disconn_set(g_client, event_disconnected, NULL);

	/* Setup the client. */
	err = gl_client_setup(g_client, ip, port, fname);
	if (err) {
		log_printf(LOG_ERROR, "Client setup failed.\n");
		goto cleanup;
	}

	/* Connect to the server. */
	err = gl_client_connect(g_client);
	if (err) {
		log_printf(LOG_ERROR, "Client failed to start.\n");
		goto cleanup;
	}

	/* Wait for it to return. */
	err = gl_client_thread_join(g_client);
	if (err) {
		log_printf(LOG_ERROR, "Client thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_client_free(g_client);
	g_client = NULL;

	return err;
}
#endif

/**
 * Lists the peers on the network.
 *
 * @param sock_addr Broadcast IP address to connect/bind to or NULL if the
 *                  address should be INADDR_BROADCAST.
 * @param verbose   Print out each step along the way?
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_list_peers(const struct sockaddr *sock_addr, bool verbose) {
	gl_err_t *err;

	err = gl_client_discover_peers(NULL);

#if 0
	/* Construct the discovery client handle object. */
	g_discovery_client = gl_client_discovery_new();
	if (g_discovery_client == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the discovery client handle object"));
	}

	/* Setup event handlers. */
	gl_client_evt_discovery_peer_set(g_discovery_client, event_peer_discovered,
									 NULL);

	/* Setup discovery socket. */
	if (sock_addr == NULL) {
		/* Use the 255.255.255.255 IP address for broadcasting. */
		err = gl_client_discovery_setup(g_discovery_client, INADDR_BROADCAST,
										UDPSERVER_PORT);
	} else {
		/* Use a specific broadcast IP address for a specific network. */
		switch (sock_addr->sa_family) {
			case AF_INET:
				err = gl_client_discovery_setup(g_discovery_client,
					((const struct sockaddr_in *)sock_addr)->sin_addr.s_addr,
					UDPSERVER_PORT);
				break;
			case AF_INET6:
			default:
				err = gl_error_new(ERR_TYPE_GL, GL_ERR_UNKNOWN,
					EMSG("Discovery service only works with IPv4"));
				break;
		}
	}

	/* Check for errors. */
	if (err) {
		log_printf(LOG_ERROR, "Discovery service setup failed.\n");
		goto cleanup;
	}

	/* Send discovery broadcast and listen to replies. */
	if (verbose)
		printf("Sending discovery broadcast...\n");
	err = gl_client_discover_peers(g_discovery_client);
	if (err) {
		log_printf(LOG_ERROR, "Discovery service thread failed to start.\n");
		goto cleanup;
	}

	/* Wait for the discovery thread to return. */
	err = gl_client_discovery_thread_join(g_discovery_client);
	if (err)
		goto cleanup;
	if (verbose)
		printf("Finished trying to discover peers.\n");

cleanup:
	/* Clean things up. */
	gl_client_discovery_free(g_discovery_client);
	g_discovery_client = NULL;
#endif

	        return err;
}

#ifndef SINGLE_IFACE_MODE
/**
 * Lists the peers throughout all of the network interfaces.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *client_list_peers_ifs(void) {
#if 0
	iface_info_list_t *if_list;
	sock_err_t tcp_err;
	gl_err_t *err;
	uint8_t i;

	/* Initialize some variables. */
	err = NULL;
	i = 0;

	/* Get the list of network interfaces. */
	tcp_err = socket_iface_info_list(&if_list);
	if (tcp_err) {
		return gl_error_new_errno(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to get the list of network interfaces"));
	}

	/* Go through the network interfaces. */
	for (i = 0; i < if_list->count; i++) {
		iface_info_t *iface;

		/* Get the network interface and print its name. */
		iface = if_list->ifaces[i];
		printf("Searching for peers on %s...\n", iface->name);

		/* Search for peers on the network. */
		err = client_list_peers(iface->brdaddr, false);
		if (err)
			goto cleanup;

		if (i < (if_list->count - 1))
			printf("\n");
	}

cleanup:
	/* Free our network interface list. */
	socket_iface_info_list_free(if_list);

	return err;
#else
	return client_list_peers(NULL, true);
#endif
}
#endif /* !SINGLE_IFACE_MODE */

#if 0
/**
 * Handles the client connected event.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
void event_connected(const tcp_client_t *client, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print some information about the current state of the connection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client connected to server on %s port %u\n", ipstr,
		   ntohs(client->addr_in.sin_port));
	printf("Waiting for the server to accept the file...\n");

	free(ipstr);
}

/**
 * Handles the client's connection request response event.
 *
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the connection been accepted by the server?
 * @param arg      Optional data set by the event handler setup.
 */
void event_conn_req_resp(const file_bundle_t *fb, bool accepted, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	if (accepted) {
		printf("Server accepted receiving %s\n", fb->base);
	} else {
		printf("Server refused receiving %s\n", fb->base);
	}
}

/**
 * Handles the client's file upload progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
void event_send_progress(const gl_client_progress_t *progress, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Sending %s (%u/%lu)\n", progress->fb->base, progress->sent_bytes,
		   progress->fb->size);
}

/**
 * Handles the client's file upload succeeded event.
 *
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
void event_send_success(const file_bundle_t *fb, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Finished sending %s\n", fb->base);
}

/**
 * Handles the client disconnection event.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
void event_disconnected(const tcp_client_t *client, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print some information about the disconnection. */
	ipstr = tcp_client_get_ipstr(client);
	printf("Client disconnected from server at %s\n", ipstr);

	free(ipstr);
}

/**
 * Handles the peer discovered event.
 *
 * @param peer Discovered peer object.
 * @param arg  Optional data set by the event handler setup.
 */
void event_peer_discovered(const gl_discovery_peer_t *peer, void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	/* Display the discovered peer. */
	printf("Discovered peer '%s' (%s)\n", peer->name,
		   inet_ntoa(peer->sock->addr_in.sin_addr));
}
#endif
