/**
 * capabilities.h
 * Helper functions to determine the capabilities of a system at runtime.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_CAPABILITIES_H
#define _GL_UTILS_CAPABILITIES_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initializer */
void cap_init(void);

#ifdef _WIN32
/* Windows-specific capabilities. */
LPOSVERSIONINFO cap_win_ver(void);
bool cap_win_least_xp(void);
bool cap_win_least_11(void);
#endif /* _WIN32 */

/* String capabilities. */
bool cap_utf8(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_CAPABILITIES_H */
