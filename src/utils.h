/**
 * utils.h
 * A collection of random utility functions to help us out.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_H
#define _GL_UTILS_H

#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

char *struntil(const char *begin, char token, const char **end);
bool parse_num(const char *str, long *num);
bool ask_yn(const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_H */
