/**
 * windows/transfer.h
 * File transfer base window.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_WINDOWS_TRANSFER_H
#define _GL_GTK_WINDOWS_TRANSFER_H

#include <gtk/gtk.h>
#include <utils/filesystem.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRANSFER_TYPE_WINDOW (transfer_window_get_type())
#define TRANSFER_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), TRANSFER_TYPE_WINDOW, TransferWindow))
#define TRANSFER_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), TRANSFER_TYPE_WINDOW, TransferWindowClass))
#define TRANSFER_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), TRANSFER_TYPE_WINDOW))
#define TRANSFER_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), TRANSFER_TYPE_WINDOW))
#define TRANSFER_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), TRANSFER_TYPE_WINDOW, TransferWindowClass))

typedef struct _TransferWindow TransferWindow;
typedef struct _TransferWindowClass TransferWindowClass;

struct _TransferWindow {
	GtkWindow parent_instance;

	fsize_t transferred;
	fsize_t target_fsize;
	gchar *target_text_cache;

	guint update_interval;

	GtkWidget *info_label;
	GtkWidget *size_label;
	GtkWidget *speed_label;

	GtkWidget *progress;

	GtkWidget *open_folder_button;
	GtkWidget *open_file_button;
	GtkWidget *cancel_button;
};

struct _TransferWindowClass {
	GtkWindowClass parent_class;
};

/* Constructor */
GType transfer_window_get_type(void);
GtkWidget *transfer_window_new(void);

/* High-level interactions. */
void transfer_window_set_update_interval(TransferWindow *window,
										 guint interval);
void transfer_window_set_target(TransferWindow *window, fsize_t target);
void transfer_window_set_progress(TransferWindow *window, fsize_t transferred);

/* Widget interactions. */
void transfer_window_set_info_label(TransferWindow *window, const gchar *text);
void transfer_window_set_progress_label(TransferWindow *window,
										fsize_t transferred);
void transfer_window_set_progress_bar_value(TransferWindow *window,
											gdouble fraction);
void transfer_window_cancel_button_hide(TransferWindow *window);
void transfer_window_open_buttons_show(TransferWindow *window);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_WINDOWS_TRANSFER_H */
