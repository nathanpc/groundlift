/**
 * windows/sendfile.c
 * Send file window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "sendfile.h"

#include <groundlift/client.h>
#include <groundlift/defaults.h>
#include <groundlift/error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../comdlg.h"

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

/* Private methods. */
static void sendfile_window_finalize(GObject *gobject);
void cancel_button_clicked(const GtkWidget *widget, gpointer data);
void tree_selection_changed(GtkTreeSelection *selection, gpointer data);
void gl_event_peer_discovered(const gl_discovery_peer_t *peer, void *arg);

/**
 * Send File window class initializer.
 *
 * @param klass Send File window class.
 */
static void sendfile_window_class_init(SendFileWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	/* Handle some standard object events. */
	object_class->finalize = sendfile_window_finalize;
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

	GtkWidget *tree;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *select;

	/* Set some defaults. */
	self->gl_discovery_client = NULL;

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
	self->file_button = button;

	/* IP address entry field. */
	hbox = gtk_hbox_new(false, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("Client Address: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
	self->ipaddr_entry = entry;

	/* Peer list tree. */
	self->peer_list_store = gtk_list_store_new(N_COLUMNS,
											   G_TYPE_STRING,
											   G_TYPE_STRING);
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(self->peer_list_store));
	g_object_unref(G_OBJECT(self->peer_list_store));
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
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed",
					 G_CALLBACK(tree_selection_changed), self);
	gtk_box_pack_start(GTK_BOX(vbox), tree, true, true, 0);
	self->peer_tree = tree;

	/* Dialog buttons. */
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_spacing(GTK_BOX(hbox), 5);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(cancel_button_clicked), self);
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
	gtk_button_set_label(GTK_BUTTON(button), "Send");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

	/* Show window and its widgets. */
	gtk_widget_show_all(window);

	/* Discover the peers on the network and populate the peer list. */
	sendfile_window_peers_discover(self);
}

/**
 * Send File window object constructor.
 *
 * @return Send File window object cast to a GtkWidget.
 */
GtkWidget *sendfile_window_new(void) {
	return GTK_WIDGET(g_object_new(SENDFILE_TYPE_WINDOW, NULL));
}

/**
 * Handle the object's destruction and frees up any resources.
 *
 * @param gobject Send File window object.
 */
static void sendfile_window_finalize(GObject *gobject) {
	/* Free up our used resources. */
	gl_client_discovery_free(SENDFILE_WINDOW(gobject)->gl_discovery_client);

	/* Always chain up to the parent class. */
	G_OBJECT_CLASS(sendfile_window_parent_class)->finalize(gobject);
}

/**
 * Sends a discovery broadcast and populates the peers list with the responses.
 *
 * @warning This function will clear the peers list before populating it.
 *
 * @param window Send File Window object.
 */
void sendfile_window_peers_discover(SendFileWindow *window) {
	iface_info_list_t *if_list;
	tcp_err_t tcp_err;
	gl_err_t *err;
	int i;

	/* Start with a clean slate. */
	err = NULL;
	sendfile_window_peers_clear(window);

	/* Get the list of network interfaces. */
	tcp_err = socket_iface_info_list(&if_list);
	if (tcp_err) {
		err = gl_error_new_errno(ERR_TYPE_TCP, (int8_t)tcp_err,
			EMSG("Failed to get the list of network interfaces"));
		goto cleanup;
	}

	/* Go through the network interfaces. */
	for (i = 0; i < if_list->count; i++) {
		/* Get the network interface and search for peers on the network. */
		sendfile_window_peers_discover_with_addr(window,
												 if_list->ifaces[i]->brdaddr);
	}

cleanup:
	/* Free our network interface list. */
	socket_iface_info_list_free(if_list);

	/* Check for errors. */
	if (err)
		dialog_error_gl(GTK_WINDOW(window), err);
}

/**
 * Sends a discovery broadcast on a specific socket address and appends the
 * responses to the peers list.
 *
 * @note This function appends peers to the list, meaning it won't clear it.
 *
 * @param window    Send File Window object.
 * @param sock_addr Broadcast IP address to connect/bind to or NULL if the
 *                  address should be INADDR_BROADCAST.
 */
void sendfile_window_peers_discover_with_addr(SendFileWindow *window,
		const struct sockaddr *sock_addr) {
	gl_err_t *err;

	/* Construct the discovery client handle object. */
	window->gl_discovery_client = gl_client_discovery_new();
	if (window->gl_discovery_client == NULL) {
		err = gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
			EMSG("Failed to construct the discovery client handle object"));
		dialog_error_gl(GTK_WINDOW(window), err);
		return;
	}

	/* Setup event handlers. */
	gl_client_evt_discovery_peer_set(window->gl_discovery_client,
									 gl_event_peer_discovered, (void *)window);

	/* Setup discovery socket. */
	if (sock_addr == NULL) {
		/* Use the 255.255.255.255 IP address for broadcasting. */
		err = gl_client_discovery_setup(window->gl_discovery_client,
										INADDR_BROADCAST, UDPSERVER_PORT);
	} else {
		/* Use a specific broadcast IP address for a specific network. */
		switch (sock_addr->sa_family) {
			case AF_INET:
				err = gl_client_discovery_setup(window->gl_discovery_client,
					((const struct sockaddr_in *)sock_addr)->sin_addr.s_addr,
					UDPSERVER_PORT);
				break;
			default:
				err = gl_error_new(ERR_TYPE_GL, GL_ERR_UNKNOWN,
					EMSG("Discovery service only works with IPv4"));
				break;
		}
	}

	/* Check for errors. */
	if (err) {
		dialog_error_gl(GTK_WINDOW(window), err);
		goto cleanup;
	}

	/* Send discovery broadcast and listen to replies. */
	err = gl_client_discover_peers(window->gl_discovery_client);
	if (err) {
		dialog_error_gl(GTK_WINDOW(window), err);
		goto cleanup;
	}

	/* Wait for the discovery thread to return. */
	err = gl_client_discovery_thread_join(window->gl_discovery_client);
	if (err) {
		dialog_error_gl(GTK_WINDOW(window), err);
		goto cleanup;
	}

cleanup:
	/* Clean things up. */
	gl_client_discovery_free(window->gl_discovery_client);
	window->gl_discovery_client = NULL;
}

/**
 * Clears the contents of the peer list.
 *
 * @param window Send File Window object.
 */
void sendfile_window_peers_clear(SendFileWindow *window) {
	gtk_list_store_clear(window->peer_list_store);
}

/**
 * Appends a peer to the peer list.
 *
 * @param window Send File Window object.
 * @param peer   Peer object to be appended to the list.
 */
void sendfile_window_peers_append(SendFileWindow *window,
								  const gl_discovery_peer_t *peer) {
	gtk_list_store_insert_with_values(window->peer_list_store, NULL, -1,
		HOSTNAME_COLUMN, peer->name,
		IPADDR_COLUMN, inet_ntoa(peer->sock->addr_in.sin_addr), -1);
}

/**
 * Appends a peer to the peer list from specific hostname and IP address values.
 *
 * @param window   Send File Window object.
 * @param hostname Peer's hostname.
 * @param ipaddr   Peer's IP address.
 */
void sendfile_window_peers_append_with_values(SendFileWindow *window,
											  const gchar *hostname,
											  const gchar *ipaddr) {
	gtk_list_store_insert_with_values(window->peer_list_store, NULL, -1,
		HOSTNAME_COLUMN, hostname, IPADDR_COLUMN, ipaddr, -1);
}

/**
 * Peer tree view selection changed event handler.
 *
 * @param selection Selected item in the peer list.
 * @param data      Pointer to the window's widget.
 */
void tree_selection_changed(GtkTreeSelection *selection, gpointer data) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *ipaddr;

	/* Get the selected item and populate the IP address entry field. */
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, IPADDR_COLUMN, &ipaddr, -1);
		gtk_entry_set_text(GTK_ENTRY(SENDFILE_WINDOW(data)->ipaddr_entry),
						   ipaddr);

		g_free(ipaddr);
	}
}

/**
 * Cancel button clicked event handler.
 *
 * @param widget Widget that triggered this event handler.
 * @param data   Pointer to the window's widget.
 */
void cancel_button_clicked(const GtkWidget *widget, gpointer data) {
	(void)widget;
	gtk_widget_destroy(GTK_WIDGET(data));
}

/**
 * Handles the peer discovered event.
 *
 * @param peer Discovered peer object.
 * @param arg  Send File Window object.
 */
void gl_event_peer_discovered(const gl_discovery_peer_t *peer, void *arg) {
	sendfile_window_peers_append(SENDFILE_WINDOW(arg), peer);
}
