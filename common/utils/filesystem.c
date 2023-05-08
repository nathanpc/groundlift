/**
 * filesystem.c
 * Some utility and helper macros to deal with the filesystem.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "filesystem.h"

#ifdef _WIN32
#include <shlobj.h>
#include <shlwapi.h>
#include <stdshim.h>
#include "utf16.h"
#else
#include <libgen.h>
#include <pwd.h>
#include <unistd.h>
#endif /* _WIN32 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Creates an empty file bundle.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @return Newly allocated file bundle object or NULL if an error occurred.
 */
file_bundle_t *file_bundle_new_empty(void) {
	file_bundle_t *fb;

	/* Allocate our file bundle. */
	fb = (file_bundle_t *)malloc(sizeof(file_bundle_t));
	if (fb == NULL)
		return NULL;

	/* Initialize the bundle with some sane defaults. */
	fb->name = NULL;
	fb->base = NULL;
	fb->size = 0;

	return fb;
}

/**
 * Creates a fully populated file bundle from a file path.
 *
 * @param fname Path to the file to get information from.
 *
 * @return Newly allocated file bundle object or NULL if an error occurred.
 */
file_bundle_t *file_bundle_new(const char *fname) {
	file_bundle_t *fb;
	sfsize_t fsize;

	/* Allocate our file bundle. */
	fb = file_bundle_new_empty();
	if (fb == NULL)
		return NULL;

	/* Get the size of the file. */
	fsize = file_size(fname);
	if (fsize < 0L) {
		fprintf(stderr, "Failed to get the length of the file %s.\n", fname);
		free(fb);

		return NULL;
	}
	fb->size = (fsize_t)fsize;

	/* Populate file names. */
	if (!file_bundle_set_name(fb, fname)) {
		free(fb);
		return NULL;
	}

	return fb;
}

/**
 * Sets the file name, and infer basename, of a file bundle. Reallocating the
 * internal strings if necessary.
 *
 * @param fb    File bundle object to be populated.
 * @param fname Path to a file.
 *
 * @return TRUE if the file bundle object population was successful.
 */
bool file_bundle_set_name(file_bundle_t *fb, const char *fname) {
	/* Set the file path. */
	if (fb->name)
		free(fb->name);
	fb->name = strdup(fname);
	if (fb->name == NULL) {
		fprintf(stderr, "Failed to duplicate %s file name string.\n", fname);
		return false;
	}

	/* Determine the file's basename. */
	if (fb->base)
		free(fb->base);
	fb->base = path_basename(fb->name);
	if (fb->base == NULL) {
		fprintf(stderr, "Failed to get %s basename.\n", fname);
		free(fb->name);
		fb->name = NULL;

		return false;
	}

	return true;
}

/**
 * Frees up any resources allocated by a file bundle.
 *
 * @param fb File bundle object.
 */
void file_bundle_free(file_bundle_t *fb) {
	/* Check if we have anything to do. */
	if (fb == NULL)
		return;

	/* File path. */
	if (fb->name) {
		free(fb->name);
		fb->name = NULL;
	}

	/* File basename. */
	if (fb->base) {
		free(fb->base);
		fb->base = NULL;
	}

	/* Free ourselves. */
	free(fb);
	fb = NULL;
}

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
sfsize_t file_size(const char *fname) {
	FILE *fh;
	fsize_t len;

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
	szPath = utf16_mbstowcs(path);
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
	char *path;

	/* Get the path to the home directory. */
#ifdef _WIN32
	TCHAR szPath[MAX_PATH + 1];

	/* Get the desktop folder. */
	if (!SHGetSpecialFolderPath(HWND_DESKTOP, szPath, CSIDL_DESKTOPDIRECTORY,
		 FALSE)) {
		/* Looks like we've failed to get the directory. Fallback to C:\. */
		return strdup("C:\\");
	}

	/* Convert the path from UTF-16 to UTF-8. */
	path = utf16_wcstombs(szPath);
#else
	char *home_path;
	const char *tmp_path;

	/* Try to get the home directory path in any way possible. */
	tmp_path = getenv("HOME");
	if (tmp_path == NULL)
		tmp_path = getpwuid(getuid())->pw_dir;

	/* Store the path to our home directory. */
	home_path = strdup(tmp_path);

	/* Check if a canonical path exists otherwise switch to using the home. */
	path_concat(&path, home_path, "Downloads", NULL);
	if (!dir_exists(path))
		path = strdup(home_path);
	free(home_path);
#endif /* _WIN32 */

	return path;
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
 * Gets the basename of the of a path. This is an implementation-agnostic
 * wrapper around the basename() function.
 *
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param path Path to have its basename extracted.
 *
 * @return Newly allocated basename of the path or NULL if an error occurred.
 *
 * @see basename
 */
char *path_basename(const char *path) {
	char *bname;

#ifdef _WIN32
	LPTSTR szPath;
	LPTSTR szBaseName;

	/* Convert the path to UTF-16 and get its basename. */
	szPath = utf16_mbstowcs(path);
	szBaseName = PathFindFileName(szPath);

	/* Convert the basename to UTF-8 and free our temporary string. */
	bname = utf16_wcstombs(szBaseName);
	free(szPath);
#else
	char *tmp;

	/* Duplicate the path just to comply with the POSIX implementation. */
	tmp = strdup(path);

	/* Get the basename and free the temporary resources. */
	bname = strdup(basename(tmp));
	free(tmp);
#endif /* _WIN32 */

	return bname;
}

/**
 * Gets the extension from a path.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param path Path to have its extension extracted from.
 *
 * @return Newly allocated extension of the path or NULL if an error occurred or
 *         if the file has no extension.
 */
char *path_extname(const char *path) {
	const char *ext;

	/* Go through the file path backwards trying to find a dot. */
	ext = path + (strlen(path) - 1);
	while ((*ext != '.') && (ext != path)) {
		ext--;
	}

	/* Check if we found an extension. */
	if ((ext == path) || (*(ext - 1) == PATH_SEPARATOR))
		return NULL;

	/* Return a duplicated extension string. */
	return strdup(ext + 1);
}

/**
 * Places a NULL terminator character in the position of the file extension dot
 * and returns the same pointer. Effectively it hides the extension from string
 * operation functions and doesn't mess around with memory.
 *
 * @param path Path to have the extension hidden.
 *
 * @return Pointer to the original path string with the extension hidden or the
 *         exact same string as before if an extension wasn't found.
 */
char *path_remove_ext(char *path) {
	char *buf;

	/* Go through the file path backwards trying to find a dot. */
	buf = path + (strlen(path) - 1);
	while ((*buf != '.') && (*buf != PATH_SEPARATOR) && (buf != path)) {
		buf--;
	}

	/* Check if we found an extension. */
	if ((buf != path) && (*buf == '.') && (*(buf - 1) != PATH_SEPARATOR))
		*buf = '\0';

	return path;
}

/**
 * Builds up a valid and unique file download path taking care of any conflicts
 * and avoiding overrides.
 *
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param dir   Path to the desired download directory.
 * @param fname Desired downloaded file name.
 *
 * @return Newly allocated valid and unique download file path or NULL if an
 *         error occurred.
 */
char *path_build_download(const char *dir, const char *fname) {
	char *path;
	char *bname;
	char *ext;
	size_t len;
	uint16_t i;

	/* Concatenate the download directory and the file name. */
	len = path_concat(&path, dir, fname, NULL);
	if (len == 0)
		return NULL;

	/* Return the file path if it doesn't exist already. */
	if (!file_exists(path))
		return path;

	/* Ensure we get a unique path. */
	i = 1;
	bname = path_remove_ext(path_basename(fname));
	ext = path_extname(fname);
	do {
		char *new_name;
		size_t nlen;

		/* Destroy the previously generated path. */
		free(path);
		path = NULL;

		/* Append a number to the filename. */
		if (ext == NULL) {
			nlen = snprintf(NULL, 0, "%s (%u)", bname, i) + 1;
			new_name = (char *)malloc(nlen * sizeof(char));
			if (new_name == NULL) {
				path = NULL;
				goto cleanup;
			}
			sprintf(new_name, "%s (%u)", bname, i);
		} else {
			nlen = snprintf(NULL, 0, "%s (%u).%s", bname, i, ext) + 1;
			new_name = (char *)malloc(nlen * sizeof(char));
			if (new_name == NULL) {
				path = NULL;
				goto cleanup;
			}
			sprintf(new_name, "%s (%u).%s", bname, i, ext);
		}

		/* Build a new path. */
		len = path_concat(&path, dir, new_name, NULL);
		if (len == 0) {
			path = NULL;
			free(new_name);

			goto cleanup;
		}

		/* Increase the index and free up any local resources. */
		free(new_name);
		i++;
	} while (file_exists(path));

cleanup:
	/* Free any temporary resources. */
	free(bname);
	free(ext);

	return path;
}
