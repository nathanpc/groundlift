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
#include <utils/utf16.h>
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
static char *conf_gethostname(void);

/**
 * Initializes the internal configuration object.
 */
void conf_init(void) {
	/* Initialize some defaults. */
	conf.download_dir = dir_defaults_downloads();
	conf.hostname = conf_gethostname();
	strncpy(conf.devtype, GL_DEVICE_TYPE, 3);
	conf.devtype[3] = '\0';

	/* Set the GLUPI. */
	/* TODO: Create this from some known factors. */
	conf.glupi[0] = 0x12;
	conf.glupi[1] = 0x34;
	conf.glupi[2] = 0x56;
	conf.glupi[3] = 0x78;
	conf.glupi[4] = 0x9A;
	conf.glupi[5] = 0xBC;
	conf.glupi[6] = 0xDE;
	conf.glupi[7] = 0xF0;
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
 * Gets the GroundLift Unique Peer Identifier for this client.
 *
 * @return GroundLift Unique Peer Identifier.
 */
const uint8_t *conf_get_glupi(void) {
	return conf.glupi;
}

/**
 * Gets the device type for glproto.
 *
 * @param buf Buffer of size 4 to hold the device type string.
 */
void conf_get_devtype(char *buf) {
	strncpy(buf, conf.devtype, 3);
	buf[3] = '\0';
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

/**
 * Gets the computer's local hostname from the operating system.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @return Newly allocated hostname string.
 */
char *conf_gethostname(void) {
#ifdef _WIN32
	TCHAR szHostname[HOST_NAME_MAX + 1];
	DWORD dwLen;

	/* Get the NetBIOS hostname. */
	dwLen = HOST_NAME_MAX + 1;
	GetComputerName(szHostname, &dwLen);

	/* Return an MBCS version of the hostname. */
	return utf16_wcstombs(szHostname);
#else
	char hostname[HOST_NAME_MAX + 1];

	/* Get the hostname and return an allocated copy of it. */
	gethostname(hostname, HOST_NAME_MAX + 1);
	return strdup(hostname);
#endif /* _WIN32 */
}
