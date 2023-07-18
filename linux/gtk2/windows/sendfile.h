/**
 * windows/sendfile.h
 * Send file window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_WINDOWS_SENDFILE_H
#define _GL_GTK_WINDOWS_SENDFILE_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENDFILE_TYPE_WINDOW (sendfile_window_get_type())
#define SENDFILE_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), SENDFILE_TYPE_WINDOW, SendFileWindow))
#define SENDFILE_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), SENDFILE_TYPE_WINDOW, SendFileWindowClass))
#define SENDFILE_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), SENDFILE_TYPE_WINDOW))
#define SENDFILE_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), SENDFILE_TYPE_WINDOW))
#define SENDFILE_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), SENDFILE_TYPE_WINDOW, SendFileWindowClass))

typedef struct _SendFileWindow SendFileWindow;
typedef struct _SendFileWindowClass SendFileWindowClass;

struct _SendFileWindow {
	GtkWindow parent_instance;
};

struct _SendFileWindowClass {
	GtkWindowClass parent_class;
};

GType sendfile_window_get_type(void);
GtkWidget *sendfile_window_new(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_WINDOWS_SENDFILE_H */
