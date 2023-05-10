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

#ifdef _WIN32

#include <windows.h>
#include <tchar.h>
#include <stdint.h>

#if _MSC_VER < 1930
	/* snprintf should be _snprintf. */
	#ifndef snprintf
		#define snprintf _snprintf
	#endif /* snprintf */
#endif

/* strdup should be _strdup. */
#ifndef strdup
	#define strdup _strdup
#endif /* strdup */

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* _GL_STD_STDSHIM_H */
