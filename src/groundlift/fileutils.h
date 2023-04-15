/**
 * fileutils.h
 * Some utility and helper macros to deal with files.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_FILEUTILS_H
#define _GL_FILEUTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Operations */
FILE *file_open(const char *fname);
ssize_t file_read(FILE *fh, void **buf, size_t len);
bool file_close(FILE *fh);

/* Checking */
int64_t file_size(const char *fname);
bool file_exists(const char *fname);

#ifdef __cplusplus
}
#endif

#endif /* _GL_FILEUTILS_H */
