/**
 * stdbool.h
 * A tiny reimplementation of the stdbool.h header for platforms that lack it.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_STD_STDBOOL_H
#define _GL_STD_STDBOOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Boolean type definition. */
#ifndef bool
typedef int bool;
#endif /* bool */

/* True and false definitions. */
#define true 1
#define false 0

#ifdef __cplusplus
}
#endif

#endif /* _GL_STD_STDBOOL_H */
