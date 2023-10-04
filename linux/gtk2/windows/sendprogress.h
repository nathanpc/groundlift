/**
 * windows/sendprogress.h
 * Send file progress window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_WINDOWS_SEND_PROGRESS_H
#define _GL_GTK_WINDOWS_SEND_PROGRESS_H

#include <groundlift/client.h>
#include <gtk/gtk.h>

#include "transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENDING_TYPE_WINDOW (sending_window_get_type())
#define SENDING_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), SENDING_TYPE_WINDOW, SendingWindow))
#define SENDING_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), SENDING_TYPE_WINDOW, SendingWindowClass))
#define SENDING_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), SENDING_TYPE_WINDOW))
#define SENDING_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), SENDING_TYPE_WINDOW))
#define SENDING_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), SENDING_TYPE_WINDOW, SendingWindowClass))

typedef struct _SendingWindow SendingWindow;
typedef struct _SendingWindowClass SendingWindowClass;

struct _SendingWindow {
	TransferWindow parent_instance;

	client_handle_t *gl_client;
};

struct _SendingWindowClass {
	TransferWindowClass parent_class;
};

/* Constructor */
GType sending_window_get_type(void);
GtkWidget *sending_window_new(void);

/* GroundLift integration. */
void sending_window_send_file(SendingWindow *window, const char *ipaddr,
							  const char *fname);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_WINDOWS_SEND_PROGRESS_H */
