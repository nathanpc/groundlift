/**
 * windows/transfer.c
 * File transfer base window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "transfer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Define GObject type. */
G_DEFINE_TYPE(TransferWindow, transfer_window, GTK_TYPE_WINDOW)

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
	TransferWindow *window;
	gpointer data;
} thread_data_t;

/* Object lifecycle. */
static void transfer_window_populate(TransferWindow *self);
static void transfer_window_constructed(GObject *gobject);
static void transfer_window_finalize(GObject *gobject);

/* Private methods. */
static void transfer_window_update_progress(TransferWindow *window,
											gboolean update_rate);
static char *get_rounded_file_size(fsize_t fsize);

/* Event handlers. */
static gboolean event_timer_update_progress(gpointer data);

/**
 * File transfer window class initializer.
 *
 * @param klass File transfer window class.
 */
static void transfer_window_class_init(TransferWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	/* Handle some standard object events. */
	object_class->constructed = transfer_window_constructed;
	object_class->finalize = transfer_window_finalize;
}

/**
 * File transfer window object initializer.
 *
 * @param self Send file window object.
 */
static void transfer_window_init(TransferWindow *self) {
	/* Set some defaults. */
	self->update_interval = 1000;
	self->transferred = 0;
	self->target_fsize = 0;
	self->target_text_cache = NULL;
}

/**
 * File transfer window object constructor.
 *
 * @return File transfer window object cast to a GtkWidget.
 */
GtkWidget *transfer_window_new(void) {
	return GTK_WIDGET(g_object_new(TRANSFER_TYPE_WINDOW, NULL));
}

/**
 * Handle the object's constructed event.
 *
 * @param gobject File transfer window object.
 */
static void transfer_window_constructed(GObject *gobject) {
	TransferWindow *self = TRANSFER_WINDOW(gobject);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(transfer_window_parent_class)->constructed(gobject);

	/* Populate the window. */
	transfer_window_populate(self);

	/* Show window and its widgets. */
	gtk_widget_show_all(GTK_WIDGET(self));
	gtk_widget_hide(self->open_folder_button);
	gtk_widget_hide(self->open_file_button);
}

/**
 * Handle the object's destruction and frees up any resources.
 *
 * @param gobject File transfer window object.
 */
static void transfer_window_finalize(GObject *gobject) {
	TransferWindow *self = TRANSFER_WINDOW(gobject);

	/* Free up our resources. */
	if (self->target_text_cache)
		free(self->target_text_cache);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(transfer_window_parent_class)->finalize(gobject);
}

/**
 * Populates the window with widgets and sets up object defaults.
 *
 * @param self File transfer window object.
 */
static void transfer_window_populate(TransferWindow *self) {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;

	GtkWidget *label;
	GtkWidget *progress;
	GtkWidget *button;

	/* Create the root window. */
	window = GTK_WIDGET(self);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 350, 110);
	gtk_window_set_title(GTK_WINDOW(window), "File transfer");
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	/* Initialize the window's root container. */
	vbox = gtk_vbox_new(false, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Transfer information labels. */
	hbox = gtk_hbox_new(false, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("Receiving whatever_file-2023.jpg from blueberry");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	self->info_label = label;

	/* Transfer progress labels. */
	hbox = gtk_hbox_new(false, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("1.83MB of 23MB");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	self->size_label = label;
	label = gtk_label_new("2.53 Mb/s");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_box_pack_end(GTK_BOX(hbox), label, false, false, 0);
	self->speed_label = label;

	/* Transfer progress bar. */
	progress = gtk_progress_bar_new();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), progress, false, false, 0);
	self->progress = progress;

	/* Dialog buttons. */
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_spacing(GTK_BOX(hbox), 5);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, false, false, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_button_set_label(GTK_BUTTON(button), "Open Folder");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	self->open_folder_button = button;
	button = gtk_button_new_from_stock(GTK_STOCK_FILE);
	gtk_button_set_label(GTK_BUTTON(button), "Open File");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	self->open_file_button = button;
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	self->cancel_button = button;
}

/**
 * Sets the window progress update interval.
 *
 * @param window   File transfer window object.
 * @param interval Time interval between progress updates.
 */
void transfer_window_set_update_interval(TransferWindow *window,
										 guint interval) {
	window->update_interval = interval;
}

/**
 * Sets the file transfer target file size.
 *
 * @param window File transfer window object.
 * @param target Target file size to be transferred.
 */
void transfer_window_set_target(TransferWindow *window, fsize_t target) {
	/* Set the internal state variables. */
	window->transferred = 0;
	window->target_fsize = target;
	window->target_text_cache = strdup(get_rounded_file_size(target));

	/* Start the progress update timer. */
	transfer_window_update_progress(window, true);
	g_timeout_add(window->update_interval, event_timer_update_progress, window);
}

/**
 * Sets the current amount of transferred bytes.
 *
 * @param window      File transfer window object.
 * @param transferred Current amount of transferred bytes.
 */
void transfer_window_set_progress(TransferWindow *window, fsize_t transferred) {
	window->transferred = transferred;
	transfer_window_update_progress(window, false);
}

/**
 * Forcefully updates all of the widgets related to the progress.
 *
 * @param window      File transfer window object.
 * @param update_rate Update the transfer rate label?
 */
static void transfer_window_update_progress(TransferWindow *window,
											gboolean update_rate) {
	static fsize_t prev_size;

	/* Calculate and display the speed of the transfer. */
	if (window->transferred == 0) {
		prev_size = 0;
		gtk_label_set_text(GTK_LABEL(window->speed_label), "");
	} else if (update_rate) {
		gchar *speed;
		fsize_t rate;

		/* Calculate the transfer rate. */
		rate = (fsize_t)((double)(window->transferred - prev_size) /
						 ((double)(window->update_interval) / 1000));
		asprintf(&speed, "%s/s", get_rounded_file_size(rate));

		/* Set the label text and free our temporary buffer. */
		gtk_label_set_text(GTK_LABEL(window->speed_label), speed);
		free(speed);

		/* Update our previous value. */
		prev_size = window->transferred;
	}

	/* Update the widgets. */
	transfer_window_set_progress_label(window, window->transferred);
	transfer_window_set_progress_bar_value(window,
		((gdouble)window->transferred / (gdouble)window->target_fsize));
}

/**
 * Sets the text of the transfer's information label (the one at the top).
 *
 * @param window File transfer window object.
 * @param text   Text to be displayed in the transfer's information label.
 */
void transfer_window_set_info_label(TransferWindow *window, const gchar *text) {
	gtk_label_set_text(GTK_LABEL(window->info_label), text);
}

/**
 * Sets the value of the progress label.
 *
 * @param window      File transfer window object.
 * @param transferred Current tally of the file that was transferred.
 */
void transfer_window_set_progress_label(TransferWindow *window,
										fsize_t transferred) {
	gchar *buf;

	/* Convert the transferred value into a human-readable string. */
	asprintf(&buf, "%s of %s", get_rounded_file_size(transferred),
			 window->target_text_cache);
	gtk_label_set_text(GTK_LABEL(window->size_label), buf);

	/* Free our temporary resources. */
	free(buf);
}

/**
 * Sets the transfer window's progress bar to a determined percentage.
 *
 * @param window   File transfer window object.
 * @param fraction Percentage of the progress.
 */
void transfer_window_set_progress_bar_value(TransferWindow *window,
											gdouble fraction) {
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress), fraction);
}

/**
 * Hides away the Cancel button.
 *
 * @param window File transfer window object.
 */
void transfer_window_cancel_button_hide(TransferWindow *window) {
	gtk_widget_hide(window->cancel_button);
}

/**
 * Shows the Open File and Folder buttons.
 *
 * @param window File transfer window object.
 */
void transfer_window_open_buttons_show(TransferWindow *window) {
	gtk_widget_show(window->open_folder_button);
	gtk_widget_show(window->open_file_button);
}

/**
 * Gets a human-readable string with a file size and its unit rounded to the
 * nearest hundred.
 *
 * @param fsize File size to be rounded.
 *
 * @return Pointer to an internal string buffer with the file size and its unit.
 */
static char *get_rounded_file_size(fsize_t fsize) {
	static gchar buf[15];
	float readable;
	char prefix;

	/* Get the human-readable file size and build up a nice string with it. */
	readable = file_size_readable(fsize, &prefix);
	if (prefix != 'B') {
		snprintf(buf, 15, "%.2f %cB", readable, prefix);
	} else {
		snprintf(buf, 15, "%.0f bytes", readable);
	}
	buf[14] = '\0';

	return buf;
}

/**
 * Handles the timer event for updating the transfer progress.
 *
 * @param data File transfer window object.
 */
static gboolean event_timer_update_progress(gpointer data) {
	TransferWindow *window = TRANSFER_WINDOW(data);
	gboolean running = gtk_widget_get_visible(window->cancel_button);

	/* Update the progress in the window. */
	if (running)
		transfer_window_update_progress(window, true);

	/* Continue updating the progress until the cancel button is hidden. */
	return running;
}
