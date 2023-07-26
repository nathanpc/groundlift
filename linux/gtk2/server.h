/**
 * server.h
 * GroundLift server GTK application helper.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_GTK_SERVER_H
#define _GL_GTK_SERVER_H

#include <groundlift/server.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <groundlift/server.h>
#include <gtk/gtk.h>

/* Daemon operations. */
gl_err_t *server_daemon_start(void);
gl_err_t *server_daemon_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_GTK_SERVER_H */
