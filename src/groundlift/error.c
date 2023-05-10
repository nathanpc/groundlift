/**
 * error.c
 * GroundLift's error handling and reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "error.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <stdshim.h>
#endif /* _WIN32 */

/**
 * Creates a brand new error reporting object.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param type Type of the error being reported.
 * @param err  Error code.
 * @param msg  Descriptive error message.
 *
 * @return Brand newly allocated, fully populated, error reporting object or
 *         NULL if we were able to allocate the necessary memory.
 *
 * @see gl_error_free
 */
gl_err_t *gl_error_new(err_type_t type, int8_t err, const char *msg) {
	return gl_error_new_prefixed(type, err, NULL, msg);
}

/**
 * Creates a brand new error reporting object from a standardized errno error
 * message.
 *
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param type   Type of the error being reported.
 * @param err    Error code.
 * @param prefix Descriptive error message prefix for the errno message string.
 *
 * @return Brand newly allocated, fully populated, error reporting object or
 *         NULL if we were able to allocate the necessary memory.
 *
 * @see gl_error_new_prefix
 * @see gl_error_free
 */
gl_err_t *gl_error_new_errno(err_type_t type, int8_t err, const char *prefix) {
	return gl_error_new_prefixed(type, err, prefix, strerror(errno));
}

/**
 * Creates a brand new error reporting object.
 * @warning This function allocates memory that must be free'd by you!
 *
 * @param type   Type of the error being reported.
 * @param err    Error code.
 * @param prefix Prefix of the error message or NULL to not include one.
 * @param msg    Descriptive error message.
 *
 * @return Brand newly allocated, fully populated, error reporting object or
 *         NULL if we were able to allocate the necessary memory.
 *
 * @see gl_error_new
 * @see gl_error_free
 */
gl_err_t *gl_error_new_prefixed(err_type_t type, int8_t err, const char *prefix,
								const char *msg) {
	gl_err_t *report;
	size_t len;

	/* Allocate memory for our error reporting structure. */
	report = (gl_err_t *)malloc(sizeof(gl_err_t));
	if (report == NULL)
		return NULL;

	/* Populate our report. */
	report->type = type;
	report->error.generic = err;

	/* Just use the message if no prefix was passed. */
	if (prefix == NULL) {
		/* Copy the message string and ensure we were able to allocate it. */
		report->msg = strdup(msg);
		if (report->msg == NULL) {
			free(report);
			return NULL;
		}

		return report;
	}

	/* Calculate the size of the string we need for the message. */
	len = strlen(msg) + strlen(prefix) + 2 + 1;

	/* Allocate memory for our message string. */
	report->msg = (char *)malloc(len * sizeof(char));
	if (report->msg == NULL) {
		free(report);
		return NULL;
	}

	/* Copy over the entire message. */
	snprintf(report->msg, len, "%s: %s", prefix, msg);

	return report;
}

/**
 * Frees up any resources allocated by an error reporting structure.
 *
 * @param err Error reporting object to be free'd.
 */
void gl_error_free(gl_err_t *err) {
	/* Do we even have anything to free? */
	if (err == NULL)
		return;

	/* Free the error message string. */
	if (err->msg)
		free(err->msg);
	err->msg = NULL;

	/* Free ourselves. */
	free(err);
	err = NULL;
}

/**
 * Substitutes the message in an error report.
 *
 * @param err Error report object.
 * @param msg New error message.
 *
 * @return Same error report object passed in err.
 */
gl_err_t *gl_error_subst_msg(gl_err_t *err, const char *msg) {
	/* Do we even have an error? */
	if (err == NULL)
		return NULL;

	/* Do we have anything in the message field currently? */
	if (err->msg)
		free(err->msg);

	/* Put the new message in. */
	err->msg = strdup(msg);

	return err;
}

/**
 * Prints out the error in detail, but only if it's actually an error.
 *
 * @param err Error reporting object.
 */
void gl_error_print(gl_err_t *err) {
	/* Check if we need to print first. */
	if ((err == NULL) || (err->error.generic == 0))
		return;

	/* Print the error out. */
	fprintf(stderr, "%s (err type %d code %d)\n", err->msg, err->type,
			err->error.generic);
}
