/**
 * fileutils.h
 * Some utility and helper macros to deal with files.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_FILEUTILS_H
#define _GL_FILEUTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* File Operations */
FILE *file_open(const char *fname, const char *mode);
ssize_t file_read(FILE *fh, void **buf, size_t len);
ssize_t file_write(FILE *fh, const void *buf, size_t len);
bool file_close(FILE *fh);

/* Checking */
int64_t file_size(const char *fname);
bool file_exists(const char *fname);

/* Directories */
bool dir_exists(const char *path);
char *dir_defaults_downloads(void);

/* Path Manipulations */
size_t path_concat(char **buf, ...);
char *path_basename(const char *path);
char *path_extname(const char *path);
char *path_remove_ext(char *path);
char *path_build_download(const char *dir, const char *fname);

#ifdef __cplusplus
}
#endif

#endif /* _GL_FILEUTILS_H */
