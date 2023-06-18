/**
 * dbus/main.c
 * GroundLift's D-Bus service.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include <groundlift/conf.h>
#include <groundlift/defaults.h>
#include <groundlift/obex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/capabilities.h>

#include "gdbus.h"

/* Private methods. */
static void on_name_acquired(GDBusConnection *connection, const gchar *name,
							 gpointer user_data);
static gboolean on_handle_send(Groundlift *skeleton,
							   GDBusMethodInvocation *invocation,
							   const gchar *ipaddr, const gchar *path);

/**
 * Program's main entry point.
 *
 * @param argc Number of command line arguments passed.
 * @param argv Command line arguments passed.
 *
 * @return Application's return code.
 */
int main(int argc, char **argv) {
	GMainLoop *loop;
	gl_err_t *err;
	int ret;

	/* Setup our defaults. */
	ret = 0;
	err = NULL;

	/* Initialize some common modules. */
	cap_init();
	obex_init();
	conf_init();

	/* Check if we had any errors to report. */
	gl_error_print(err);
	if (err != NULL) {
		ret = 1;
		goto cleanup;
	}

	/* Setup GLib and the D-Bus subsystem. */
	loop = g_main_loop_new(NULL, false);
	g_bus_own_name(G_BUS_TYPE_SESSION, "com.innoveworkshop.Groundlift",
				   G_BUS_NAME_OWNER_FLAGS_NONE, NULL, on_name_acquired,
				   NULL, NULL, NULL);

	/* Enter the GLib main loop. */
	g_main_loop_run(loop);

cleanup:
	/* Free up any resources. */
	//gl_server_free(g_server);
	//gl_client_free(g_client);
	gl_error_free(err);
	conf_free();

	return ret;
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
							 gpointer user_data) {
	Groundlift *skeleton;

	skeleton = groundlift_skeleton_new();
	g_signal_connect(skeleton, "handle-send", G_CALLBACK(on_handle_send), NULL);

	/* Export the interface based on the provided skeleton. */
	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton),
		connection, "/com/innoveworkshop/Groundlift", NULL);
}

static gboolean on_handle_send(Groundlift *skeleton,
							   GDBusMethodInvocation *invocation,
							   const gchar *ipaddr, const gchar *path) {
	groundlift_complete_send(skeleton, invocation);
	return G_DBUS_METHOD_INVOCATION_HANDLED;
}
