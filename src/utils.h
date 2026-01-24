/**
 * utils.h
 * A collection of random utility functions to help us out.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_UTILS_H
#define _GL_UTILS_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* String manipulation. */
char *struntil(const char *begin, char token, const char **end);
bool parse_num(const char *str, long *num);
bool parse_size(const char *str, size_t *num);

/* User interaction. */
bool ask_yn(const char *msg, ...);
void buffered_progress(const char *name, size_t acc, size_t fsize);
size_t read_stdin(char **text, bool trim);

/* File system. */
int fname_sanitize(char *fname);
size_t file_size(const char *fname);
bool file_exists(const char *fname);
char *path_basename(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTILS_H */
