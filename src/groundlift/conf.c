/**
 * conf.c
 * GroundLift user configuration.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "conf.h"

#ifdef _WIN32
#include <winsock.h>
#include <stdshim.h>
#else
#include <unistd.h>
#endif /* _WIN32 */
#include <limits.h>
#include <string.h>
#include <utils/filesystem.h>

#include "defaults.h"

/* Private variables. */
static gl_conf_t conf;

/* Private methods. */
char *conf_gethostname(void);

/**
 * Initializes the internal configuration object.
 */
void conf_init(void) {
	/* Initialize some defaults. */
	conf.download_dir = dir_defaults_downloads();
	conf.hostname = conf_gethostname();
}

/**
 * Frees up any resources allocated by the configuration module.
 */
void conf_free(void) {
	/* Downloads directory. */
	if (conf.download_dir) {
		free(conf.download_dir);
		conf.download_dir = NULL;
	}

	/* Machine's name. */
	if (conf.hostname) {
		free(conf.hostname);
		conf.hostname = NULL;
	}
}

/**
 * Gets the computer's local hostname from the operating system.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @return Newly allocated hostname string.
 */
char *conf_gethostname(void) {
	char hostname[HOST_NAME_MAX + 1];

	/* Get the hostname. */
	gethostname(hostname, HOST_NAME_MAX + 1);

	/* Return an allocated copy of the hostname. */
	return strdup(hostname);
}

/**
 * Gets the configured hostname.
 *
 * @return Machine's local name.
 */
const char *conf_get_hostname(void) {
	return conf.hostname;
}

/**
 * Gets the configured download directory.
 *
 * @return Path to the download directory.
 */
const char *conf_get_download_dir(void) {
	return conf.download_dir;
}
