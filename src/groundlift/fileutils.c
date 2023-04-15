/**
 * fileutils.c
 * Some utility and helper macros to deal with files.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "fileutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Opens a file in binary mode.
 *
 * @param fname Path to the file to be opened.
 *
 * @return File handle or NULL if an error occurred while opening.
 */
FILE *file_open(const char *fname) {
	FILE *fh;

	/* Open the file and check for errors. */
	fh = fopen(fname, "rb");
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

	/* Allocate some memory to hold the contents of the file. */
	*buf = malloc(len);
	if (*buf == NULL)
		return -1L;

	/* Read some of the contents of the file into the buffer. */
	nread = fread(*buf, 1, len, fh);
	if (ferror(fh)) {
		free(*buf);
		*buf = NULL;

		return -1L;
	}

	return nread;
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
	fh = file_open(fname);
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
	return access(fname, F_OK) != -1;
#endif /* _WIN32 */
}
