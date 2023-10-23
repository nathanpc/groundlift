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
#include <utils/logging.h>
#ifdef _WIN32
	#include <stdshim.h>
	#include <utils/utf16.h>
#else
	#include <signal.h>
#endif /* _WIN32 */

#include "sockets.h"

/* Private variables. */
static gl_err_t *gl_last_error;

/* Private methods. */
static void gl_error_raise_exception(void);

/**
 * Initializes the error reporting subsystem.
 */
void gl_error_init(void) {
	gl_last_error = NULL;
}

/**
 * Pushes a new error report to the error stack.
 *
 * @param type Type of the error being reported.
 * @param err  Error code.
 * @param msg  Descriptive error message.
 *
 * @return Latest error reporting object or NULL if malloc failed.
 *
 * @see gl_error_pop
 * @see gl_error_clear
 */
gl_err_t *gl_error_push(err_type_t type, int8_t err, const char *msg) {
	/* Check if it's just an event or warning that's happening. */
	if (err <= 0) {
		log_msg(LOG_WARNING, msg);
		return NULL;
	}

	return gl_error_push_prefix(type, err, NULL, msg);
}

/**
 * Creates an error report from a standardized errno error message.
 *
 * @param type Type of the error being reported.
 * @param err  Error code.
 * @param msg  Descriptive error message prefix for the errno message string.
 *
 * @return Latest error reporting object or NULL if malloc failed.
 *
 * @see gl_error_push_sockerr
 * @see gl_error_push_prefix
 */
gl_err_t *gl_error_push_errno(err_type_t type, int8_t err, const char *msg) {
	/* Check if it's just an event or warning that's happening. */
	if (err <= 0) {
		log_errno(LOG_WARNING, msg);
		return NULL;
	}

	return gl_error_push_prefix(type, err, msg, strerror(errno));
}

/**
 * Creates an error report from a standardized socket errno error message.
 *
 * @param err Error code.
 * @param msg Descriptive error message prefix for the errno message string.
 *
 * @return Latest error reporting object or NULL if malloc failed.
 *
 * @see gl_error_push_errno
 * @see gl_error_push_prefix
 */
gl_err_t *gl_error_push_sockerr(sock_err_t err, const char *msg) {
	/* Check if it's just an event that's happening. */
	if (err <= SOCK_OK) {
		log_sockerrno(LOG_WARNING, msg, sockerrno);
		return NULL;
	}

#ifdef _WIN32
	LPTSTR szErrorMessage;
	char *strMessage;

	/* Get the descriptive error message from the system. */
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, err, FORMAT_MESSAGE_LANG,
					   (LPTSTR)&szErrorMessage, 0, NULL)) {
		szErrorMessage = _wcsdup(_T("FormatMessage failed"));
	}

	/* Convert the message to UTF-8 and push the error to the stack. */
	strMessage = utf16_wcstombs(szErrorMessage);
	gl_error_push_prefix(ERR_TYPE_SOCKET, err, msg, strMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
	free(strMessage);

	return gl_last_error;
#else
	return gl_error_push_prefix(ERR_TYPE_SOCKET, err, msg, strerror(errno));
#endif /* _WIN32 */
}

/**
 * Pushes a new error report to the error stack with a prefix to the message.
 *
 * @param type   Type of the error being reported.
 * @param err    Error code.
 * @param prefix Prefix of the error message or NULL to not include one.
 * @param msg    Descriptive error message.
 *
 * @return Latest error reporting object or NULL if malloc failed.
 *
 * @see gl_error_push
 * @see gl_error_pop
 * @see gl_error_clear
 */
gl_err_t *gl_error_push_prefix(err_type_t type, int8_t err, const char *prefix,
							   const char *msg) {
	gl_err_t *report;
	size_t len;

	/* Allocate memory for our error reporting structure. */
	report = (gl_err_t *)malloc(sizeof(gl_err_t));
	if (report == NULL)
		return NULL;

	/* Populate our report. */
	report->type = type;
	report->code.generic = err;

	/* Do the stack switcheroo. */
	report->prev = (void *)gl_last_error;
	gl_last_error = report;

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

	/* Raise a software exception. */
	gl_error_raise_exception();

	return gl_last_error;
}

/**
 * Frees up any resources allocated by an error reporting object.
 *
 * @param err Error reporting object to be free'd.
 *
 * @return Previous error in the stack or NULL if no more errors exist.
 */
gl_err_t *gl_error_pop(gl_err_t *err) {
	/* Do we even have anything to free? */
	if (err == NULL)
		return NULL;

	/* Backtrack the last error stack pointer. */
	gl_last_error = (gl_err_t *)err->prev;

	/* Free the error message string. */
	if (err->msg)
		free(err->msg);
	err->msg = NULL;

	/* Free ourselves. */
	free(err);
	err = NULL;

	return gl_last_error;
}

/**
 * Clears the entire error stack.
 */
void gl_error_clear(void) {
	while (gl_last_error != NULL)
		gl_error_pop(gl_last_error);
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
 * @warning This function will clear the error history stack.
 *
 * @param err Error reporting object.
 */
void gl_error_print(gl_err_t *err) {
	/* Check if we need to print first. */
	if ((err == NULL) || (err->code.generic == 0))
		return;

	/* Print the error stack out. */
	do {
		log_printf(LOG_ERROR, "%s (err type %d code %d)\n", err->msg, err->type,
				   err->code.generic);
		err = gl_error_pop(err);
	} while (err != NULL);
}

/**
 * Raises a software exception in debug builds.
 */
void gl_error_raise_exception(void) {
#ifdef DEBUG
#ifdef _WIN32
	__debugbreak();
#else
	raise(SIGTRAP);
#endif /* _WIN32 */
#endif /* DEBUG */
}
