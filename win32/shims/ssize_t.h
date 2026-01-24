/**
 * ssize_t.h
 * A definition of ssize_t for platforms that lack it.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_STD_SSIZE_T_H
#define _GL_STD_SSIZE_T_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ssize_t
	#ifdef _WIN32
		#include <windows.h>
		typedef LONG_PTR ssize_t;
	#endif /* _WIN32 */
#endif /* ssize_t */

#ifdef __cplusplus
}
#endif

#endif /* _GL_STD_SSIZE_T_H */