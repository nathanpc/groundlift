/**
 * comdlg.c
 * Common GTK message dialog helpers.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "comdlg.h"
#include "groundlift/error.h"

/**
 * Displays a modal error dialog associated with a GroundLift error.
 *
 * @warning This function will free the error object automatically.
 *
 * @param parent Parent window or NULL if no parent should be specified.
 * @param err    GroundLift error object. (Automatically free'd by us)
 */
void dialog_error_gl(GtkWindow *parent, gl_err_t *err) {
	GtkWidget *dialog;

	/* Setup the message dialog and display it. */
	dialog = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
									"Internal GroundLift Error");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
											 "%s\nType: %d\nCode: %d", err->msg,
											 err->type, err->error.generic);
	gtk_dialog_run(GTK_DIALOG(dialog));

	/* Free our resources. */
	gtk_widget_destroy(dialog);
	gl_error_free(err);
}
