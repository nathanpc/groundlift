/**
 * windows/sendfile.c
 * Send file window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "sendfile.h"

#include <groundlift/defaults.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Define GObject type. */
G_DEFINE_TYPE(SendFileWindow, sendfile_window, GTK_TYPE_WINDOW)

/**
 * Columns of the peers Tree.
 */
typedef enum {
	HOSTNAME_COLUMN = 0,
	IPADDR_COLUMN,
	N_COLUMNS
} peer_list_cols_t;

/**
 * Send File window class initializer.
 *
 * @param klass Send File window class.
 */
static void sendfile_window_class_init(SendFileWindowClass *klass) {
	/* This space was intentionally left blank. */
}

/**
 * Send File window object initializer.
 *
 * @param self Send file window object.
 */
static void sendfile_window_init(SendFileWindow *self) {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *hbox;

	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *button;

	GtkTreeStore *store;
	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* Create the root window. */
	window = GTK_WIDGET(self);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 350, 300);
	gtk_window_set_title(GTK_WINDOW(window), "Send File");
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	/* Initialize the window's root container. */
	vbox = gtk_vbox_new(false, 5);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* File to send selection. */
	hbox = gtk_hbox_new(false, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("File to send: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	button = gtk_file_chooser_button_new("File to Send",
										 GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

	/* IP address entry field. */
	hbox = gtk_hbox_new(false, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("Client Address: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);

	/* Peer list tree. */
	store = gtk_tree_store_new(N_COLUMNS,
							   G_TYPE_STRING,
							   G_TYPE_STRING);
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Hostname", renderer,
													  "text", HOSTNAME_COLUMN,
													  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("IP Address", renderer,
													  "text", IPADDR_COLUMN,
													  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	gtk_box_pack_start(GTK_BOX(vbox), tree, true, true, 0);

	/* Dialog buttons. */
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_spacing(GTK_BOX(hbox), 5);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	button = gtk_button_new_with_label("Close");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	button = gtk_button_new_with_label("Send");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

	/* Show window and its widgets. */
	gtk_widget_show_all(window);
}

/**
 * Send File window object constructor.
 *
 * @return Send File window object cast to a GtkWidget.
 */
GtkWidget *sendfile_window_new(void) {
	return GTK_WIDGET(g_object_new(SENDFILE_TYPE_WINDOW, NULL));
}
