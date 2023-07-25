/**
 * linux/gtk2/main.c
 * GroundLift's GTK+ 2 application.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/conf.h>
#include <groundlift/defaults.h>
#include <groundlift/obex.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/capabilities.h>

#include "server.h"
#include "windows/sendfile.h"

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	GtkWidget *window;
	gl_err_t *err;
	int ret;

	/* Setup our defaults. */
	ret = 0;
	err = NULL;

	/* Initialize some common modules. */
	cap_init();
	obex_init();
	conf_init();

	/* Start the server daemon. */
	err = server_daemon_start();
	if (err != NULL) {
		gl_error_print(err);
		ret = 1;

		goto cleanup;
	}

	/* Initialize GTK and create the root window. */
	gtk_init(&argc, &argv);
	window = sendfile_window_new();
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit),
					 G_OBJECT(window));

	/* Enter GTK's main loop. */
	gtk_main();

cleanup:
	err = server_daemon_stop();
	if (err != NULL) {
		gl_error_print(err);
		ret = 1;
	}

	/* Free up any resources. */
	gl_error_free(err);
	conf_free();

	return ret;
}
