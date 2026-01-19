/**
 * utils.c
 * A collection of random utility functions to help us out.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utils.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <unistd.h>
	#include <libgen.h>
#endif /* _WIN32 */
#include <time.h>

#include "defaults.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "logging.h"

/**
 * Gets a string from begin to token without including the token.
 *
 * If the end of the string is reached before the token is found, the entire
 * string is returned as a new allocated string (equivalent to strdup).
 *
 * @warning This function allocates the return string that must later be freed.
 *
 * @param begin Beginning of the string to search for the token.
 * @param token Token to search for in the string.
 * @param end   Optional. Returns the position in begin right after the token
 *              was found (skips the token), unless it's the NUL terminator.
 *
 * @return Newly allocated, NUL terminated, string with the contents from begin
 *         to token. Returns NULL when begin is the end of the string.
 */
char *struntil(const char *begin, char token, const char **end) {
	const char *cur;
	const char *ctmp;
	char *buf;
	char *btmp;

	/* Have we reached the end? */
	if (*begin == '\0')
		return NULL;

	/* Search for the token or the end of the string. */
	cur = begin;
	while ((*cur != token) && (*cur != '\0'))
		cur++;

	/* Return the position where the token was found. */
	if (end != NULL)
		*end = (*cur == '\0') ? cur : cur + 1;

	/* Allocate memory for our new string. */
	buf = (char *)malloc((cur - begin + 1) * sizeof(char));
	if (buf == NULL) {
		log_syserr(LOG_CRIT, "Failed to allocate memory for token string");
		return NULL;
	}

	/* Copy our string over. */
	btmp = buf;
	ctmp = begin;
	while (ctmp != cur) {
		*btmp = *ctmp;
		ctmp++;
		btmp++;
	}
	*btmp = '\0';

	return buf;
}

/**
 * Converts a string to a number and indicates in case of a failure.
 *
 * @remark This function will set errno on failure according to atol or strtol
 *         on your system.
 *
 * @param str String to be converted to a number.
 * @param num Number to return if the conversion is successful.
 *
 * @return TRUE if the conversion was successful, FALSE otherwise.
 */
bool parse_num(const char *str, long *num) {
#ifdef WITHOUT_STRTOL
	*num = atol(str);
#else
	*num = strtol(str, NULL, 10);
#endif /* WITHOUT_STRTOL */

	return (*num != LONG_MIN) && (*num != LONG_MAX) && ((*num != 0) ||
		(strcmp(str, "0") == 0));
}

/**
 * Converts a string to a size_t and indicates in case of a failure. This is a
 * wrapper over parse_num.
 *
 * @remark This function will set errno on failure according to atol or strtol
 *         on your system.
 *
 * @param str String to be converted to a number.
 * @param num Number to return if the conversion is successful.
 *
 * @return TRUE if the conversion was successful, FALSE otherwise.
 *
 * @see parse_num
 */
bool parse_size(const char *str, size_t *num) {
	long l;
	bool ret;

	/* Parse the number. */
	ret = parse_num(str, &l);
	*num = ret ? l : 0;

	return ret;
}

/**
 * Prompts the user to answer a yes or no question. Defaults to yes.
 *
 * @param msg Question to be asked to the user.
 * @param ... Additional variables to be populated in the question.
 *
 * @return TRUE if the answer was yes, FALSE otherwise.
 */
bool ask_yn(const char *msg, ...) {
	va_list args;
	char resp;
	int c;

	/* Print the message and options for the user. */
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, " [Y/n] ");

	/* Get an answer from the user and flush the input until a newline. */
	resp = '\0';
	while ((c = getchar()) != '\n' && c != EOF) {
		if (resp == '\0')
			resp = (char)c;
	}

	return (resp == 'y') || (resp == 'Y') || (resp == '\0');
}

/**
 * Buffers the transfer progress in order to improve the performance when
 * printing to the console.
 *
 * @warning This function uses an internal static variable to keep track of the
 *          progress and is thus not thread-safe.
 *
 * @param name  Name of the object being transferred.
 * @param acc   Accumulated size so far.
 * @param fsize Final size of the transfer.
 */
void buffered_progress(const char *name, size_t acc, size_t fsize) {
	static time_t elapsed;
	time_t now;
	bool print;

	/* Print the progress every 500ms. */
	now = time(NULL);
	print = (difftime(now, elapsed) > 0.5);

	/* Always print between transfers or when it's finished. */
	if ((acc <= RECV_BUF_LEN) || (acc >= fsize))
		print = true;

	/* Is it time to print? */
	if (print) {
		fprintf(stderr, "\r%s (%lu/%lu)", name, acc, fsize);
		elapsed = now;
	}
}

/**
 * Sanitizes a file name to ensure idiots don't abuse us.
 *
 * @param fname File name to be sanitized.
 *
 * @return Number of characters that were altered.
 */
int fname_sanitize(char *fname) {
	int ret;
	char *buf;

	ret = 0;
	buf = fname;
	while (*buf != '\0') {
		if (((*buf == '.') && (*(buf + 1) == '.')) || (*buf == '/') ||
				(*buf == '\\')) {
			*buf = '_';
			ret++;
			break;
		}

		buf++;
	}

	return ret;
}

/**
 * Gets the size of an entire file.
 *
 * @param fname Path to the file to be inspected.
 *
 * @return The size of the file in bytes or 0 if an error occurred.
 */
size_t file_size(const char *fname) {
	FILE *fh;
	size_t len;

	/* Open the file. */
	fh = fopen(fname, "rb");
	if (fh == NULL)
		return 0L;

	/* Seek to the end of the file to determine its size. */
	fseek(fh, 0L, SEEK_END);
	len = ftell(fh);
	fclose(fh);

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

	/* Should we even check? */
	if (fname == NULL)
		return 0;

	/* Get file attributes and return. */
	dwAttrib = GetFileAttributes(fname);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		   !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
#else
	/* Should we even check? */
	if (fname == NULL)
		return 0;

	return access(fname, F_OK) != -1;
#endif /* _WIN32 */
}

/**
 * Gets the basename of a path. This is an implementation-agnostic wrapper
 * around the basename() function.
 *
 * @warning This function allocates memory that must be freed by you!
 *
 * @param path Path to have its basename extracted.
 *
 * @return Newly allocated basename of the path or NULL if an error occurred.
 */
char *path_basename(const char *path) {
	char *bname;

#ifdef _WIN32
	bname = strdup(PathFindFileName(path));
#else
	/* Duplicate the path just to comply with the POSIX implementation. */
	char *tmp = strdup(path);

	/* Get the basename and free the temporary resources. */
	bname = strdup(basename(tmp));
	free(tmp);
#endif /* _WIN32 */

	return bname;
}
