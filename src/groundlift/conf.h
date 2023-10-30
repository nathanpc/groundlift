/**
 * conf.h
 * GroundLift user configuration.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CONF_H
#define _GL_CONF_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configuration object.
 */
typedef struct {
	uint8_t glupi[8];
	char devtype[4];
	char *hostname;

	char *download_dir;
} gl_conf_t;

/* Initialization and destruction. */
void conf_init(void);
void conf_free(void);

/* Getters */
const uint8_t *conf_get_glupi(void);
void conf_get_devtype(char *buf);
const char *conf_get_hostname(void);
const char *conf_get_download_dir(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CONF_H */
