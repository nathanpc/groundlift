/**
 * fileutils.c
 * Some utility and helper macros to deal with files.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "fileutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "defaults.h"

/**
 * Opens a file in binary mode.
 *
 * @param fname Path to the file to be opened.
 * @param mode  File operation mode.
 *
 * @return File handle or NULL if an error occurred while opening.
 */
FILE *file_open(const char *fname, const char *mode) {
	FILE *fh;

	/* Open the file and check for errors. */
	fh = fopen(fname, mode);
	if (fh == NULL)
		perror("file_open");

	return fh;
}

/**
 * Reads part of the contents of a file.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param fh  Opened file handle.
 * @param buf Buffer to place the contents of the file into. (Allocated by this
 *            function)
 * @param len Number of bytes to read from the file.
 *
 * @return Number of bytes read from the file or -1 in case of an error.
 *
 * @see file_size
 */
ssize_t file_read(FILE *fh, void **buf, size_t len) {
	ssize_t nread;

	/* Ensure that we can in fact read something from the file. */
	if (fh == NULL)
		return -1L;

	/* Allocate some memory to hold the contents of the file. */
	*buf = malloc(len);
	if (*buf == NULL)
		return -1L;

	/* Read some of the contents of the file into the buffer. */
	nread = fread(*buf, sizeof(uint8_t), len, fh);
	if (ferror(fh)) {
		free(*buf);
		*buf = NULL;

		return -1L;
	}

	return nread;
}

/**
 * Writes some bytes to a file.
 *
 * @param fh  Opened file handle.
 * @param buf Buffer to place the contents of into the file.
 * @param len Size of the buffer in bytes.
 *
 * @return Number of bytes written to the file or -1 if an error occurred.
 */
ssize_t file_write(FILE *fh, const void *buf, size_t len) {
	ssize_t n;

	/* Ensure that we can write to the file. */
	if (fh == NULL)
		return -1L;

	/* Do we even have anything to write? */
	if ((buf == NULL) || (len == 0))
		return 0;

	/* Write the buffer contents to the file. */
	n = fwrite(buf, sizeof(uint8_t), len, fh);
	if (n == 0)
		return -1L;

	return n;
}

/**
 * Closes a file handle.
 *
 * @param fh File handle to be closed.
 *
 * @return TRUE if the operation was successful.
 */
bool file_close(FILE *fh) {
	return fclose(fh) == 0;
}

/**
 * Gets the size of an entire file.
 *
 * @param fname Path to the file to be inspected.
 *
 * @return The size of the file in bytes or -1 if an error occurred.
 */
int64_t file_size(const char *fname) {
	FILE *fh;
	long len;

	/* Open the file. */
	fh = file_open(fname, "rb");
	if (fh == NULL) {
		return -1L;
	}

	/* Seek to the end of the file to determine its size. */
	fseek(fh, 0L, SEEK_END);
	len = ftell(fh);

	/* Close the file handle and return its size. */
	file_close(fh);
	return len;
}

/**
 * Checks if a file exists.
 *
 * @param  fname File path to be checked.
 *
 * @return TRUE if the file exists.
 */
bool file_exists(const char *fname) {
#ifdef _WIN32
	DWORD dwAttrib;
	LPTSTR szPath;

	/* Should we even check? */
	if (fname == NULL)
		return false;

	/* Convert the file path to UTF-16. */
	szPath = utf16_mbstowcs(fname);
	if (szPath == NULL)
		return false;

	/* Get file attributes and return. */
	dwAttrib = GetFileAttributes(szPath);
	LocalFree(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		   !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	/* Should we even check? */
	if (fname == NULL)
		return false;

	return access(fname, F_OK) != -1;
#endif /* _WIN32 */
}

/**
 * Checks if a directory exists and ensures it's actually a directory.
 *
 * @param path Path to a directory to be checked.
 *
 * @return TRUE if the path represents an existing directory.
 */
bool dir_exists(const char *path) {
#ifdef _WIN32
	DWORD dwAttrib;
	LPTSTR szPath;

	/* Should we even check? */
	if (path == NULL)
		return false;

	/* Convert the file path to UTF-16. */
	szPath = utf16_mbstowcs(fname);
	if (szPath == NULL)
		return false;

	/* Get file attributes and return. */
	dwAttrib = GetFileAttributes(szPath);
	LocalFree(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		   (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat sb;

	/* Should we even check? */
	if (path == NULL)
		return false;

	/* Ensure that we can stat the path. */
	if (stat(path, &sb) < 0)
		return false;

	/* Check if it's actually a directory. */
	return S_ISDIR(sb.st_mode);
#endif /* _WIN32 */
}

/**
 * Gets the default directory to store downloads.
 *
 * @return String containing the system's default locations of the user's
 *         downloads folder.
 */
char *dir_defaults_downloads(void) {
	/* TODO: Properly handle this. */
	return strdup("./downloads");
}

/**
 * Concatenates paths together safely.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param buf Pointer to final path string. (Allocated internally)
 * @param ... Paths to be concatenated. WARNING: A NULL must be placed at the
 *            end to indicate the end of the list.
 *
 * @return Size of the final buffer or 0 if an error occurred.
 */
size_t path_concat(char **buf, ...) {
	va_list ap;
	size_t len;
	const char *path;
	char *cur;

	/* Initialize things and ensure we leave space for the NULL terminator. */
	*buf = NULL;
	len = 1;

	/* Go through the paths. */
	va_start(ap, buf);
	path = va_arg(ap, char *);
	while (path != NULL) {
		size_t plen;

		/* Reallocate the buffer memory and set the cursor for concatenation. */
		plen = len;
		len += strlen(path);
		*buf = (char *)realloc(*buf, (len + 1) * sizeof(char));
		if (*buf == NULL)
			return 0L;
		cur = (*buf) + plen - 1;

		/* Should we bother prepending the path separator? */
		if ((plen > 1) && (*(cur - 1) != PATH_SEPARATOR)) {
			*cur = PATH_SEPARATOR;
			cur++;
			len++;
		}

		/* Concatenate the next path. */
		while (*path != '\0') {
			*cur = *path;

			cur++;
			path++;
		}

		/* Ensure we NULL terminate the string. */
		*cur = '\0';

		/* Fetch the next path. */
		path = va_arg(ap, char *);
	}
	va_end(ap);

	return len;
}

/**
 * Builds up a valid and unique file download path taking care of any conflicts
 * and avoiding overrides.
 *
 * @param dir   Path to the desired download directory.
 * @param fname Desired downloaded file name.
 *
 * @return Valid and unique download file path or NULL if an error occurred.
 */
char *path_build_download(const char *dir, const char *fname) {
	char *path;
	size_t len;

	/* Concatenate the download directory and the file name. */
	len = path_concat(&path, dir, fname, NULL);
	if (len == 0)
		return NULL;

#if 0
	/* Ensure we get a unique path. */
	while (file_exists(path)) {
		/* Destroy the previously generated path. */
		free(path);
		path = NULL;

		/* TODO: Build a new path. */
	}
#endif

	return path;
}
