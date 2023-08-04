/**
 * windows/recvprogress.h
 * Receiving file progress window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_WINDOWS_RECEIVE_PROGRESS_H
#define _GL_GTK_WINDOWS_RECEIVE_PROGRESS_H

#include <groundlift/server.h>
#include <gtk/gtk.h>
#include <utils/filesystem.h>

#include "transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECEIVING_TYPE_WINDOW (receiving_window_get_type())
#define RECEIVING_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), RECEIVING_TYPE_WINDOW, ReceivingWindow))
#define RECEIVING_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), RECEIVING_TYPE_WINDOW, ReceivingWindowClass))
#define RECEIVING_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), RECEIVING_TYPE_WINDOW))
#define RECEIVING_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), RECEIVING_TYPE_WINDOW))
#define RECEIVING_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), RECEIVING_TYPE_WINDOW, ReceivingWindowClass))

typedef struct _ReceivingWindow ReceivingWindow;
typedef struct _ReceivingWindowClass ReceivingWindowClass;

struct _ReceivingWindow {
	TransferWindow parent_instance;

	server_handle_t *gl_server;
	const server_conn_t *conn;

	gchar *fpath;
};

struct _ReceivingWindowClass {
	TransferWindowClass parent_class;
};

/* Constructor */
GType receiving_window_get_type(void);
GtkWidget *receiving_window_new(void);

/* GroundLift integration. */
void receiving_window_set_gl_server(ReceivingWindow *window,
									server_handle_t *gl_server);
void receiving_window_set_gl_server_conn_req(ReceivingWindow *window,
											 const gl_server_conn_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_WINDOWS_RECEIVE_PROGRESS_H */
