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

/**
 * System-agnostic representation of the size of a file.
 */
typedef uint64_t fsize_t;
typedef int64_t sfsize_t;

/**
 * Compilation of all of the common properties of a file.
 */
typedef struct {
	char *name;
	char *base;
	fsize_t size;
} file_bundle_t;

/* File Bundles */
file_bundle_t *file_bundle_new_empty(void);
file_bundle_t *file_bundle_new(const char *fname);
bool file_bundle_set_name(file_bundle_t *fb, const char *fname);
void file_bundle_free(file_bundle_t *fb);

/* File Operations */
FILE *file_open(const char *fname, const char *mode);
ssize_t file_read(FILE *fh, void **buf, size_t len);
ssize_t file_write(FILE *fh, const void *buf, size_t len);
bool file_close(FILE *fh);

/* Checking */
sfsize_t file_size(const char *fname);
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
