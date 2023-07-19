/**
 * comdlg.h
 * Common GTK message dialog helpers.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_COMMON_DIALOGS_H
#define _GL_GTK_COMMON_DIALOGS_H

#include <groundlift/error.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

void dialog_error_gl(GtkWindow *parent, gl_err_t *err);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_COMMON_DIALOGS_H */
