/**
 * error.c
 * GroundLift's error handling and reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "error.h"

#include <stdio.h>
#include <string.h>

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
 */
gl_err_t *gl_error_new(err_type_t type, int8_t err, const char *msg) {
	gl_err_t *report;

	/* Allocate memory for our error reporting structure. */
	report = (gl_err_t *)malloc(sizeof(gl_err_t));
	if (report == NULL)
		return NULL;

	/* Populate our report. */
	report->type = type;
	report->error.generic = err;

	/* Allocate memory and copy our message string. */
	report->msg = strdup(msg);
	if (report->msg == NULL) {
		free(report);
		return NULL;
	}

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
 * Prints out the error in detail, but only if it's actually an error.
 *
 * @param err Error reporting object.
 */
void gl_error_print(gl_err_t *err) {
	/* Check if we need to print first. */
	if ((err == NULL) || (err->error.generic == 0))
		return;

	/* Print the error out. */
	fprintf(stderr, "%s (%d/%d)\n", err->msg, err->type, err->error.generic);
}
