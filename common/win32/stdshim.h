/**
 * stdshim.h
 * A collection of shims to make MSVC a tiny bit more standards-compliant, even
 * if it's just visually.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_STD_STDSHIM_H
#define _GL_STD_STDSHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef _WIN32

/* snprintf should be _snprintf. */
#ifndef snprintf
	#define snprintf _snprintf
#endif /* snprintf */

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* _GL_STD_STDSHIM_H */
