/**
 * server.c
 * GroundLift server GTK application helper.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "server.h"

#include <groundlift/defaults.h>
#include <stdio.h>
#include <utils/filesystem.h>
#include <utils/logging.h>

#include "windows/recvprogress.h"

/**
 * Transfer request thread data structure.
 */
typedef struct {
	const gl_server_conn_req_t *req;
	gint response;
	gboolean running;
} conn_req_thread_data_t;

/* Server instance. */
static server_handle_t *gl_server;

/* Server operations. */
static gl_err_t *server_run(const char *ip, uint16_t port);
static void server_setup_receiving_window(const gl_server_conn_req_t *req);

/* Server event handlers. */
static void gl_event_started(const server_t *server, void *arg);
static void gl_event_stopped(void *arg);

/* Server client's connection event handlers. */
static void gl_event_conn_accepted(const server_conn_t *conn, void *arg);
static gboolean event_conn_req_thread_wrapper(gpointer data);
static int gl_event_conn_req(const gl_server_conn_req_t *req, void *arg);

/**
 * Starts up the server daemon.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *server_daemon_start(void) {
	/* TODO: Start the server on all interfaces. */
	return server_run(NULL, TCPSERVER_PORT);
}

/**
 * Stops the server daemon.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
gl_err_t *server_daemon_stop(void) {
	gl_err_t *err = NULL;

	/* Stop the server instance. */
	err = gl_server_stop(gl_server);
	if (err)
		gl_error_print(err);

	/* Wait for the discovery server thread to return. */
	err = gl_server_discovery_thread_join(gl_server);
	if (err) {
		log_printf(LOG_ERROR,
				   "Discovery server thread returned with errors.\n");
		goto cleanup;
	}

	/* Wait for the main server thread to return. */
	err = gl_server_thread_join(gl_server);
	if (err) {
		log_printf(LOG_ERROR, "Server thread returned with errors.\n");
		goto cleanup;
	}

cleanup:
	/* Free up any resources. */
	gl_server_free(gl_server);

	return err;
}

/**
 * Starts up the server and wait for it to be shutdown.
 *
 * @param ip   Server's IP to send the data to.
 * @param port Server port to talk to.
 *
 * @return An error report if something unexpected happened or NULL if the
 *         operation was successful.
 */
static gl_err_t *server_run(const char *ip, uint16_t port) {
	gl_err_t *err = NULL;

	/* Construct the server handle object. */
	gl_server = gl_server_new();
	if (gl_server == NULL) {
		return gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the server handle object."));
	}

	/* Setup event handlers. */
	gl_server_evt_start_set(gl_server, gl_event_started, NULL);
	gl_server_evt_client_conn_req_set(gl_server, gl_event_conn_req, NULL);
	gl_server_evt_stop_set(gl_server, gl_event_stopped, NULL);
	gl_server_conn_evt_accept_set(gl_server, gl_event_conn_accepted, NULL);

	/* Initialize the server. */
	err = gl_server_setup(gl_server, ip, port);
	if (err) {
		log_printf(LOG_ERROR, "Server setup failed.\n");
		goto cleanup;
	}

	/* Start the main server up. */
	err = gl_server_start(gl_server);
	if (err) {
		log_printf(LOG_ERROR, "Server thread failed to start.\n");
		goto cleanup;
	}

	/* Start the discovery server. */
	err = gl_server_discovery_start(gl_server, UDPSERVER_PORT);
	if (err) {
		log_printf(LOG_ERROR, "Discovery server thread failed to start.\n");
		goto cleanup;
	}

	/* Check if the server started successfully. */
	if (err == NULL)
		return NULL;

cleanup:
	/* Free up any resources. */
	gl_server_free(gl_server);

	return err;
}

/**
 * Sets up and displays the receiving transfer progress window.
 *
 * @param req Information about the client and its request.
 */
static void server_setup_receiving_window(const gl_server_conn_req_t *req) {
	GtkWidget *window;

	/* Setup the transfer window object. */
	window = receiving_window_new();
	receiving_window_set_gl_server(RECEIVING_WINDOW(window), gl_server);
	receiving_window_set_gl_server_conn_req(RECEIVING_WINDOW(window), req);
}

/**
 * GLib thread-safe wrapper function to handle a peer's transfer request.
 *
 * @param data conn_req_thread_data_t object.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_conn_req_thread_wrapper(gpointer data) {
	conn_req_thread_data_t *td;
	GtkWidget *dialog;
	float fsize;
	char prefix;

	/* Get thread data object. */
	td = (conn_req_thread_data_t *)data;

	/* Get a human-readable file size. */
	fsize = file_size_readable(td->req->fb->size, &prefix);

	/* Setup the message dialog. */
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
									"%s wants to send you a file. Accept?",
									td->req->hostname);

	/* Add some information about the file to be transferred. */
	if (prefix != 'B') {
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
			"%s (%.2f %cB)", td->req->fb->base, fsize, prefix);
	} else {
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
												 "%s (%.0f bytes)",
												 td->req->fb->base, fsize);
	}

	/* Ask user about the transfer and start the progress window if needed. */
	td->response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (td->response == GTK_RESPONSE_YES)
		server_setup_receiving_window(td->req);

	/* Free our resources. */
	gtk_widget_destroy(dialog);
	td->running = false;

	return G_SOURCE_REMOVE;
}

/**
 * Handles the server started event.
 *
 * @param server Server handle object.
 * @param arg    Optional data set by the event handler setup.
 */
static void gl_event_started(const server_t *server, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print some information about the current state of the server. */
	ipstr = tcp_server_get_ipstr(server);
	printf("Server listening on %s port %u (discovery %u)\n", ipstr,
		   ntohs(server->tcp.addr_in.sin_port),
		   ntohs(server->udp.addr_in.sin_port));

	free(ipstr);
}

/**
 * Handles the server client connection requested event.
 *
 * @param req Information about the client and its request.
 * @param arg Optional data set by the event handler setup.
 *
 * @return 0 to refuses the request. Anything else will be treated as accepting.
 */
static int gl_event_conn_req(const gl_server_conn_req_t *req, void *arg) {
	conn_req_thread_data_t *td;
	int response;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(conn_req_thread_data_t));
	td->req = req;
	td->running = true;
	td->response = GTK_RESPONSE_NO;

	/* Notifies the user using a thread wrapper function. */
	g_idle_add(event_conn_req_thread_wrapper, (void *)td);
	while (td->running)
		sleep(1);

	/* Get the user's response and free the thread wrapper. */
	response = (td->response == GTK_RESPONSE_YES) ? 1 : 0;
	free(td);

	return response;
}

/**
 * Handles the server stopped event.
 *
 * @param arg Optional data set by the event handler setup.
 */
static void gl_event_stopped(void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Server stopped\n");
}

/**
 * Handles the server connection accepted event.
 *
 * @param conn Client connection handle object.
 * @param arg  Optional data set by the event handler setup.
 */
static void gl_event_conn_accepted(const server_conn_t *conn, void *arg) {
	char *ipstr;

	/* Ignore unused arguments. */
	(void)arg;

	/* Print out some client information. */
	ipstr = tcp_server_conn_get_ipstr(conn);
	printf("Client at %s connection accepted\n", ipstr);

	free(ipstr);
}
