/**
 * windows/sendprogress.c
 * Send file progress window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "sendprogress.h"

#include <groundlift/defaults.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/logging.h>
#include "transfer.h"

/* Define GObject type. */
G_DEFINE_TYPE(SendingWindow, sending_window, TRANSFER_TYPE_WINDOW)

/**
 * A helper structure to pass the window object along with some data to another
 * thread function.
 */
typedef struct {
	SendingWindow *window;
	gpointer data;
} thread_data_t;

/* Object lifecycle. */
static void sending_window_constructed(GObject *gobject);
static void sending_window_finalize(GObject *gobject);

/* GroundLift event handler GTK thread wrappers. */
static gboolean event_transfer_accepted(gpointer data);
static gboolean event_transfer_refused(gpointer data);
static gboolean event_transfer_progress(gpointer data);
static gboolean event_transfer_success(gpointer data);
static gboolean event_transfer_cancelled(gpointer data);

/* GroundLift event handlers. */
static void gl_event_conn_req_resp(const file_bundle_t *fb, bool accepted,
								   void *arg);
static void gl_event_send_progress(const gl_client_progress_t *progress,
								   void *arg);
static void gl_event_send_success(const file_bundle_t *fb, void *arg);
static void gl_event_disconnected(const tcp_client_t *client, void *arg);

/* Widget event handlers. */
static void cancel_button_clicked(const GtkWidget *widget, gpointer data);

/**
 * File transfer window class initializer.
 *
 * @param klass File transfer window class.
 */
static void sending_window_class_init(SendingWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	/* Handle some standard object events. */
	object_class->constructed = sending_window_constructed;
	object_class->finalize = sending_window_finalize;
}

/**
 * File transfer window object initializer.
 *
 * @param self Send file window object.
 */
static void sending_window_init(SendingWindow *self) {
	/* Set some defaults. */
	self->gl_client = gl_client_new();
	if (self->gl_client == NULL) {
		log_errno(LOG_ERROR,
				  EMSG("Failed to construct the client handle object"));
	}
}

/**
 * File transfer window object constructor.
 *
 * @return File transfer window object cast to a GtkWidget.
 */
GtkWidget *sending_window_new(void) {
	return GTK_WIDGET(g_object_new(SENDING_TYPE_WINDOW, NULL));
}

/**
 * Handle the object's constructed event.
 *
 * @param gobject File transfer window object.
 */
static void sending_window_constructed(GObject *gobject) {
	SendingWindow *self = SENDING_WINDOW(gobject);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(sending_window_parent_class)->constructed(gobject);

	/* Setup event handlers. */
	gl_client_evt_conn_req_resp_set(self->gl_client, gl_event_conn_req_resp,
									self);
	gl_client_evt_put_progress_set(self->gl_client, gl_event_send_progress,
								   self);
	gl_client_evt_put_succeed_set(self->gl_client, gl_event_send_success, self);
	gl_client_evt_disconn_set(self->gl_client, gl_event_disconnected, self);

	/* Setup buttons event handlers. */
	g_signal_connect(G_OBJECT(self->parent_instance.cancel_button), "clicked",
					 G_CALLBACK(cancel_button_clicked), self);
}

/**
 * Handle the object's destruction and frees up any resources.
 *
 * @param gobject File transfer window object.
 */
static void sending_window_finalize(GObject *gobject) {
	SendingWindow *self = SENDING_WINDOW(gobject);
	gl_err_t *err = NULL;

	/* Wait for client thread to return. */
	err = gl_client_thread_join(self->gl_client);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Client thread returned with errors.\n");

		gl_error_free(err);
	}

	/* Free our resources. */
	if (self->gl_client)
		gl_client_free(self->gl_client);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(sending_window_parent_class)->finalize(gobject);
}

/**
 * Displays the window and starts the process of sending the file to the peer.
 *
 * @param window File transfer window object.
 * @param ip     Server's IP to send the data to.
 * @param fname  Path to a file to be sent to the server.
 */
void sending_window_send_file(SendingWindow *window, const char *ipaddr,
							  const char *fname) {
	gl_err_t *err = NULL;

	/* Setup the client. */
	err = gl_client_setup(window->gl_client, ipaddr, TCPSERVER_PORT, fname);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Client setup failed.\n");

		gl_error_free(err);
		gl_client_free(window->gl_client);

		gtk_widget_destroy(GTK_WIDGET(window));
	}

	/* Connect to the server. */
	err = gl_client_connect(window->gl_client);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Client failed to start.\n");

		gl_error_free(err);
		gl_client_free(window->gl_client);

		gtk_widget_destroy(GTK_WIDGET(window));
	}

	/* Indicate that we are waiting for a response from the server. */
	transfer_window_set_info_label(TRANSFER_WINDOW(window),
								   "Waiting for a response...");
	gtk_label_set_text(GTK_LABEL(TRANSFER_WINDOW(window)->size_label), "");
	gtk_label_set_text(GTK_LABEL(TRANSFER_WINDOW(window)->speed_label), "");
	transfer_window_set_progress_bar_value(TRANSFER_WINDOW(window), 0.0);

	/* Show window and its widgets. */
	gtk_widget_show_all(GTK_WIDGET(window));
	gtk_widget_hide(window->parent_instance.open_folder_button);
	gtk_widget_hide(window->parent_instance.open_file_button);
}

/**
 * GTK transfer request accepted response event handler.
 *
 * @param data Thread data object with window object.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_transfer_accepted(gpointer data) {
	thread_data_t *td = (thread_data_t *)data;
	file_bundle_t *fb = (file_bundle_t *)td->data;
	gchar *buf = NULL;

	/* Set the information label text. */
	asprintf(&buf, "Sending %s", fb->base);
	transfer_window_set_info_label(TRANSFER_WINDOW(td->window), buf);
	free(buf);

	/* Set the progress target and reset the progress bar. */
	transfer_window_set_target(TRANSFER_WINDOW(td->window), fb->size);
	transfer_window_set_progress(TRANSFER_WINDOW(td->window), 0);

	/* Free our resources. */
	free(fb);
	free(td);

	return G_SOURCE_REMOVE;
}

/**
 * GTK transfer request refused response event handler.
 *
 * @param data Thread data object with window object.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_transfer_refused(gpointer data) {
	thread_data_t *td = (thread_data_t *)data;

	/* Set the information label and hide the cancel button. */
	transfer_window_set_info_label(TRANSFER_WINDOW(td->window),
									"Peer declined our file transfer");
	transfer_window_cancel_button_hide(TRANSFER_WINDOW(td->window));

	/* Free our resources. */
	free(td);

	return G_SOURCE_REMOVE;
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
	fsize_t *sent_bytes = (fsize_t *)td->data;

	/* Update the progress in the window. */
	transfer_window_set_progress(TRANSFER_WINDOW(td->window),
								 *sent_bytes);

	/* Free our resources. */
	free(sent_bytes);
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
	asprintf(&buf, "Sent %s", fb->base);
	gtk_window_set_title(GTK_WINDOW(td->window), buf);
	free(buf);
	buf = NULL;

	/* Set the information label. */
	asprintf(&buf, "Sent %s successfully", fb->base);
	transfer_window_set_info_label(TRANSFER_WINDOW(td->window), buf);
	free(buf);
	buf = NULL;

	/* Hide the cancel button. */
	transfer_window_cancel_button_hide(TRANSFER_WINDOW(td->window));

	/* Free our resources. */
	file_bundle_free(fb);
	free(td);

	return G_SOURCE_REMOVE;
}

/**
 * GTK transfer cancelled event handler.
 *
 * @param data Thread data object with window object.
 *
 * @return G_SOURCE_REMOVE to mark this operation as finalized.
 */
static gboolean event_transfer_cancelled(gpointer data) {
	thread_data_t *td = (thread_data_t *)data;
	gboolean running;

	/* Check if the transfer was canceled or simply finished. */
	running = gtk_widget_get_visible(td->window->parent_instance.cancel_button);
	if (!running)
		return G_SOURCE_REMOVE;

	/* Set the information label. */
	transfer_window_set_info_label(TRANSFER_WINDOW(td->window),
								   "Transfer cancelled by the receiver");

	/* Hide the cancel button and free our resources. */
	transfer_window_cancel_button_hide(TRANSFER_WINDOW(td->window));
	free(td);

	return G_SOURCE_REMOVE;
}

/**
 * Handles the client's connection request response event.
 *
 * @param fb       File bundle that was uploaded.
 * @param accepted Has the connection been accepted by the server?
 * @param arg      Optional data set by the event handler setup.
 */
static void gl_event_conn_req_resp(const file_bundle_t *fb, bool accepted,
								   void *arg) {
	thread_data_t *td;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = SENDING_WINDOW(arg);
	td->data = NULL;

	/* Proccess the event accordingly. */
	if (accepted) {
		td->data = file_bundle_dup(fb);
		g_idle_add(event_transfer_accepted, (void *)td);
	} else {
		g_idle_add(event_transfer_refused, (void *)td);
	}
}

/**
 * Handles the client's file upload progress event.
 *
 * @param progress Structure containing all the information about the progress.
 * @param arg      Optional data set by the event handler setup.
 */
static void gl_event_send_progress(const gl_client_progress_t *progress,
								   void *arg) {
	thread_data_t *td;
	fsize_t *fs;

	/* Allocate and bytes transferred variable. */
	fs = malloc(sizeof(fsize_t));
	*fs = progress->sent_bytes;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = SENDING_WINDOW(arg);
	td->data = (gpointer)fs;

	/* Appends the peer to the peer list using a wrapper function. */
	g_idle_add(event_transfer_progress, (void *)td);
}

/**
 * Handles the client's file upload succeeded event.
 *
 * @param fb  File bundle that was uploaded.
 * @param arg Optional data set by the event handler setup.
 */
static void gl_event_send_success(const file_bundle_t *fb, void *arg) {
	thread_data_t *td;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = SENDING_WINDOW(arg);
	td->data = file_bundle_dup(fb);

	/* Appends the peer to the peer list using a wrapper function. */
	g_idle_add(event_transfer_success, (void *)td);
}

/**
 * Handles the client disconnection event.
 *
 * @param client Client connection handle object.
 * @param arg    Optional data set by the event handler setup.
 */
static void gl_event_disconnected(const tcp_client_t *client, void *arg) {
	thread_data_t *td;

	/* Ignore unused arguments. */
	(void)client;

	/* Prepares the data to be passed to the thread wrapper. */
	td = malloc(sizeof(thread_data_t));
	td->window = SENDING_WINDOW(arg);
	td->data = NULL;

	/* Appends the peer to the peer list using a wrapper function. */
	g_idle_add(event_transfer_cancelled, (void *)td);
}

/**
 * Cancel button clicked event handler.
 *
 * @param widget Widget that triggered this event handler.
 * @param data   Pointer to the window's widget.
 */
static void cancel_button_clicked(const GtkWidget *widget, gpointer data) {
	(void)widget;

	/* Hide the cancel button. */
	transfer_window_cancel_button_hide(TRANSFER_WINDOW(data));

	/* Cancel the transfer and set the information label. */
	gl_client_disconnect(SENDING_WINDOW(data)->gl_client);
	transfer_window_set_info_label(TRANSFER_WINDOW(data),
								   "Transfer cancelled by user");
}
