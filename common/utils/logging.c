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

/* Terminal color escape code definitions. */
#define TC_RED   "\x1B[31m"
#define TC_GRN   "\x1B[32m"
#define TC_YEL   "\x1B[33m"
#define TC_BLU   "\x1B[34m"
#define TC_MAG   "\x1B[35m"
#define TC_CYN   "\x1B[36m"
#define TC_WHT   "\x1B[37m"
#define TC_RST "\x1B[0m"

/**
 * Logs a generic message.
 *
 * @param level Severity of the logged information.
 * @param msg   Message to be associated with the log.
 */
void log_msg(log_level_t level, const char *msg) {
	log_printf(level, "%s\n", msg);
}

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
	log_printf(level, "%s: (%d) %ls\n", msg, dwLastError, szErrorMessage);

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
	log_printf(level, "%s: WSAError (%d) %ls\n", msg, err, szErrorMessage);

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
			fprintf(stderr, TC_RED "[FATAL] ");
			break;
		case LOG_ERROR:
			fprintf(stderr, TC_RED "[ERROR] ");
			break;
		case LOG_WARNING:
			fprintf(stderr, TC_YEL "[WARNING] ");
			break;
		case LOG_INFO:
			fprintf(stderr, TC_WHT "[INFO] ");
			break;
		case LOG_DEBUG:
			fprintf(stderr, TC_WHT "[DEBUG] ");
			break;
		default:
			fprintf(stderr, TC_MAG "[UNKNOWN] ");
			break;
	}

	/* Print the actual message. */
	va_start(args, format);
	fprintf(stderr, TC_RST);
	vfprintf(stderr, format, args);
	va_end(args);
}
