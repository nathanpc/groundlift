/**
 * windows/recvprogress.c
 * Receiving file progress window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "recvprogress.h"

#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "transfer.h"

/* Define GObject type. */
G_DEFINE_TYPE(ReceivingWindow, receiving_window, TRANSFER_TYPE_WINDOW)

/**
 * Columns of the peers Tree.
 */
typedef enum {
	HOSTNAME_COLUMN = 0,
	IPADDR_COLUMN,
	N_COLUMNS
} peer_list_cols_t;

/**
 * A helper structure to pass the window object along with some data to another
 * thread function.
 */
typedef struct {
	ReceivingWindow *window;
	gpointer data;
} thread_data_t;

/* Object lifecycle. */
static void receiving_window_constructed(GObject *gobject);
static void receiving_window_finalize(GObject *gobject);

/* GroundLift event handler GTK thread wrappers. */
static gboolean event_transfer_progress(gpointer data);
static gboolean event_transfer_success(gpointer data);

/* GroundLift event handlers. */
static void gl_event_download_progress(
	const gl_server_conn_progress_t *progress, void *arg);
static void gl_event_download_success(const file_bundle_t *fb, void *arg);
static void gl_event_conn_closed(void *arg);

/* Widget event handlers. */
static void open_file_button_clicked(const GtkWidget *widget, gpointer data);
static void open_folder_button_clicked(const GtkWidget *widget, gpointer data);

/**
 * File transfer window class initializer.
 *
 * @param klass File transfer window class.
 */
static void receiving_window_class_init(ReceivingWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	/* Handle some standard object events. */
	object_class->constructed = receiving_window_constructed;
	object_class->finalize = receiving_window_finalize;
}

/**
 * File transfer window object initializer.
 *
 * @param self Send file window object.
 */
static void receiving_window_init(ReceivingWindow *self) {
	/* Set some defaults. */
	self->gl_server = NULL;
	self->conn = NULL;
	self->fpath = NULL;
}

/**
 * File transfer window object constructor.
 *
 * @return File transfer window object cast to a GtkWidget.
 */
GtkWidget *receiving_window_new(void) {
	return GTK_WIDGET(g_object_new(RECEIVING_TYPE_WINDOW, NULL));
}

/**
 * Handle the object's constructed event.
 *
 * @param gobject File transfer window object.
 */
static void receiving_window_constructed(GObject *gobject) {
	ReceivingWindow *self = RECEIVING_WINDOW(gobject);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(receiving_window_parent_class)->constructed(gobject);

	/* Setup buttons event handlers. */
	g_signal_connect(G_OBJECT(self->parent_instance.open_file_button),
					 "clicked", G_CALLBACK(open_file_button_clicked), self);
	g_signal_connect(G_OBJECT(self->parent_instance.open_folder_button),
					 "clicked", G_CALLBACK(open_folder_button_clicked), self);
}

/**
 * Handle the object's destruction and frees up any resources.
 *
 * @param gobject File transfer window object.
 */
static void receiving_window_finalize(GObject *gobject) {
	ReceivingWindow *self = RECEIVING_WINDOW(gobject);

	/* Free our resources. */
	if (self->fpath)
		free(self->fpath);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(receiving_window_parent_class)->finalize(gobject);
}

/**
 * Sets the server object reference for the window and sets up some event
 * handlers.
 *
 * @param window File transfer window object.
 * @param server GroundLift server handle object.
 */
void receiving_window_set_gl_server(ReceivingWindow *window,
									server_handle_t *server) {
	/* Store our server handle. */
	window->gl_server = server;

	/* Setup event handlers. */
	gl_server_conn_evt_download_progress_set(server,
											 gl_event_download_progress,
											 window);
	gl_server_conn_evt_download_success_set(server,
											gl_event_download_success, window);
	gl_server_conn_evt_close_set(server, gl_event_conn_closed, window);
}

/**
 * Sets the server connection object reference for the window and sets up the
 * interface to start the transfer.
 *
 * @param window File transfer window object.
 * @param req    Information about the client and its request.
 */
void receiving_window_set_gl_server_conn_req(ReceivingWindow *window,
											 const gl_server_conn_req_t *req) {
	gchar *buf;

	/* Store our connection handle. */
	window->conn = req->conn;

	/* Set the window title. */
	buf = NULL;
	asprintf(&buf, "Receiving %s", req->fb->base);
	gtk_window_set_title(GTK_WINDOW(window), buf);
	free(buf);
	buf = NULL;

	/* Set the information label. */
	asprintf(&buf, "Receiving %s from %s", req->fb->base, req->hostname);
	transfer_window_set_info_label(TRANSFER_WINDOW(window), buf);
	free(buf);
	buf = NULL;

	/* Set the target of the transfer. */
	transfer_window_set_target(TRANSFER_WINDOW(window), req->fb->size);
}

/**
 * GTK progress update event handler.
 *
 * @param data Thread data object with window object and a pointer to the amount
 *             of bytes transferred so far.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_transfer_progress(gpointer data) {
	thread_data_t *td = (thread_data_t *)data;
	fsize_t *recv_bytes = (fsize_t *)td->data;

	/* Update the progress in the window. */
	transfer_window_set_progress(TRANSFER_WINDOW(td->window),
								 *recv_bytes);

	/* Free our resources. */
	free(recv_bytes);
	free(td);

	return G_SOURCE_REMOVE;
}

/**
 * GTK progress update event handler.
 *
 * @param data Thread data object with window and a file bundle objects.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_transfer_success(gpointer data) {
	thread_data_t *td = (thread_data_t *)data;
	file_bundle_t *fb = (file_bundle_t *)td->data;
	gchar *buf;

	/* Set the window title. */
	buf = NULL;
	asprintf(&buf, "Received %s", fb->base);
	gtk_window_set_title(GTK_WINDOW(td->window), buf);
	free(buf);
	buf = NULL;

	/* Set the information label. */
	asprintf(&buf, "Received %s successfully", fb->base);
	transfer_window_set_info_label(TRANSFER_WINDOW(td->window), buf);
	free(buf);
	buf = NULL;

	/* Set the downloaded file path. */
	td->window->fpath = strdup(fb->name);

	/* Hide the cancel button and show the open buttons. */
	transfer_window_open_buttons_show(TRANSFER_WINDOW(td->window));
	transfer_window_cancel_button_hide(TRANSFER_WINDOW(td->window));

	/* Free our resources. */
	file_bundle_free(fb);
	free(td);

	return G_SOURCE_REMOVE;
}

/**
 * Handles the server connection download progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
static void gl_event_download_progress(
		const gl_server_conn_progress_t *progress, void *arg) {
	thread_data_t *td;
	fsize_t *fs;

	/* Allocate and bytes transferred variable. */
	fs = malloc(sizeof(fsize_t));
	*fs = progress->recv_bytes;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = RECEIVING_WINDOW(arg);
	td->data = (gpointer)fs;

	/* Appends the peer to the peer list using a wrapper function. */
	g_idle_add(event_transfer_progress, (void *)td);
}

/**
 * Handles the server connection download finished event.
 *
 * @param fb  File bundle of the downloaded file.
 * @param arg Optional data set by the event handler setup.
 */
static void gl_event_download_success(const file_bundle_t *fb, void *arg) {
	thread_data_t *td;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = RECEIVING_WINDOW(arg);
	td->data = file_bundle_dup(fb);

	/* Appends the peer to the peer list using a wrapper function. */
	g_idle_add(event_transfer_success, (void *)td);
}

/**
 * Handles the server connection closed event.
 *
 * @param arg Optional data set by the event handler setup.
 */
static void gl_event_conn_closed(void *arg) {
	/* Ignore unused arguments. */
	(void)arg;

	printf("Client connection closed\n");
}

/**
 * Open File button clicked event handler.
 *
 * @param widget Widget that triggered this event handler.
 * @param data   Pointer to the window's widget.
 */
static void open_file_button_clicked(const GtkWidget *widget, gpointer data) {
	GError *error = NULL;
	gchar *uri;
	(void)widget;

	/* Open the file using the default program. */
	asprintf(&uri, "file://%s", RECEIVING_WINDOW(data)->fpath);
	if (!g_app_info_launch_default_for_uri(uri, NULL, &error)) {
		g_warning("Failed to open file URI: %s", error->message);
	}
	free(uri);

	/* Close ourselves. */
	gtk_widget_destroy(GTK_WIDGET(data));
}

/**
 * Open Folder button clicked event handler.
 *
 * @param widget Widget that triggered this event handler.
 * @param data   Pointer to the window's widget.
 */
static void open_folder_button_clicked(const GtkWidget *widget, gpointer data) {
	GError *error = NULL;
	gchar *uri;
	(void)widget;

	/* Open the folder containing the file. */
	asprintf(&uri, "file://%s", dirname(RECEIVING_WINDOW(data)->fpath));
	if (!g_app_info_launch_default_for_uri(uri, NULL, &error)) {
		g_warning("Failed to open folder URI: %s", error->message);
	}
	free(uri);

	/* Close ourselves. */
	gtk_widget_destroy(GTK_WIDGET(data));
}
