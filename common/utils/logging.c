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
#ifdef _WIN32
#include <tchar.h>
#endif /* _WIN32 */

#ifdef _WIN32
/* FormatMessage default flags. */
#define FORMAT_MESSAGE_FLAGS \
	(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM)

/* FormatMessage default language. */
#define FORMAT_MESSAGE_LANG MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
#endif /* _WIN32 */

/**
 * Logs any system errors that set the errno variable whenever dealing with
 * sockets.
 *
 * @param level Severity of the logged information.
 * @param msg   Error message to be associated with the error message determined
 *              by the errno value.
 */
void log_errno(log_level_t level, const char *msg) {
#ifdef _WIN32
	DWORD dwLastError;
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	dwLastError = GetLastError();
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, dwLastError,
					   FORMAT_MESSAGE_LANG, (LPTSTR)&szErrorMessage, 0,
					   NULL)) {
		szErrorMessage = _wcsdup(_T("FormatMessage failed"));
	}

	/* Print the error message. */
	log_printf(level, "%s: (%d) %ls", msg, dwLastError, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	/* Print the error message. */
	log_printf(level, "%s: (%d) %s\n", msg, errno, strerror(errno));
#endif /* _WIN32 */
}

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
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, err, FORMAT_MESSAGE_LANG,
					   (LPTSTR)&szErrorMessage, 0, NULL)) {
		szErrorMessage = _wcsdup(_T("FormatMessage failed"));
	}

	/* Print the error message. */
	log_printf(level, "%s: WSAError (%d) %ls", msg, err, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	(void)err;

	/* Print the error message. */
	log_printf(level, "%s: %s\n", msg, strerror(errno));
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
