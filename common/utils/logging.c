/**
 * logging.c
 * GroundLift's logging and log reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "logging.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * Logs any system errors that set the errno variable whenever dealing with
 * sockets.
 *
 * @param level Severity of the logged information.
 * @param msg   Error message to be associated with the error message determined
 *              by the errno value.
 * @param err   errno or equivalent error code value. (Ignored on POSIX systems)
 */
void log_sockerrno(log_level_t level, const char *msg, int err) {
#ifdef _WIN32
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
						   FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   &szErrorMessage, 0, NULL)) {
		szErrorMessage = _tcsdup(_T("FormatMessage failed"));
	}

	/* Print the error message. */
	log_printf("%s: WSAError (%d) %ls", msg, err, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	(void)err;

	/* Print the error message. */
	log_printf(level, "%s: %s\n", msg, strerror(errno));
	perror(msg);
#endif /* _WIN32 */
}

/**
 * Prints out logging information with an associated log level tag using the
 * printf style of function.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag.
 * @param ...    Additional variables to be populated.
 */
void log_printf(log_level_t level, const char *format, ...) {
	va_list args;

	/* Print the log level tag. */
	switch (level) {
		case LOG_FATAL:
			fprintf(stderr, "[FATAL] ");
			break;
		case LOG_ERROR:
			fprintf(stderr, "[ERROR] ");
			break;
		case LOG_WARNING:
			fprintf(stderr, "[WARNING] ");
			break;
		case LOG_INFO:
			fprintf(stderr, "[INFO] ");
			break;
		default:
			fprintf(stderr, "[UNKNOWN] ");
			break;
	}

	/* Print the actual message. */
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
