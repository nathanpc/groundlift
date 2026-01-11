/**
 * utils.c
 * A collection of random utility functions to help us out.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "utils.h"

#include <stdlib.h>
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
