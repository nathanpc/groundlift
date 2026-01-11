/**
 * logging.c
 * Logging and log reporting utility.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "logging.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif /* _WIN32 */

#include <stdio.h>
#include <string.h>
#ifdef WITH_LOG_TIME
	#include <time.h>
#endif /* WITH_LOG_TIME */
#ifndef _WIN32
	#include <errno.h>
#endif /* !_WIN32 */

/**
 * Prints out logging information with an associated log level tag using the
 * printf function style.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag.
 * @param ap     Additional variables to be populated.
 */
void log_vprintf(log_level_t level, const char *format, va_list ap) {
#ifdef WITH_LOG_TIME
	char ts[23];

	/* Time and date. */
	time_t tm = time(NULL);
	struct tm* gmt = gmtime(&tm);
	if (strftime(ts, 22, "%Y-%m-%dT%H:%M:%SZ", gmt) == 0)
		ts[20] = '?';
	ts[21] = ' ';
	ts[22] = '\0';
#else
	char ts[1];
	*ts = '\0';
#endif /* WITH_LOG_TIME */

	/* Print the log level tag. */
	switch (level) {
		case LOG_CRIT:
			printf("%s[CRITICAL] ", ts);
			break;
		case LOG_ERROR:
			printf("%s[ERROR]    ", ts);
			break;
		case LOG_WARNING:
			printf("%s[WARNING]  ", ts);
			break;
		case LOG_NOTICE:
			printf("%s[NOTICE]   ", ts);
			break;
		case LOG_INFO:
			printf("%s[INFO]     ", ts);
			break;
		default:
			printf("%s[UNKNOWN]  ", ts);
			break;
	}

	/* Print the actual message. */
	vprintf(format, ap);
}

/**
 * Prints out logging information with an associated log level tag using the
 * printf function style. Automatically appends a newline at the end of the
 * message.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag.
 * @param ...    Additional variables to be populated.
 */
void log_printf(log_level_t level, const char *format, ...) {
	va_list args;

	/* Print the log message. */
	va_start(args, format);
	log_vprintf(level, format, args);
	va_end(args);

	/* Ensure we finish with a newline. */
	printf("\n");
}

/**
 * Prints out logging information related to system errors that set the errno
 * variable. System error message is automatically appended at the end.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag or system message.
 * @param ...    Additional variables to be populated.
 */
void log_syserr(log_level_t level, const char *format, ...) {
	va_list args;
	int err;

#ifdef _WIN32
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	err = GetLastError();
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, err, FORMAT_MESSAGE_LANG,
					   (LPTSTR)&szErrorMessage, 0, NULL)) {
		szErrorMessage = strdup("FormatMessage failed");
	}

	/* Print the application's error message. */
	va_start(args, format);
	log_vprintf(level, format, args);
	va_end(args);

	/* Print the system error message. */
	printf(": System Error (%d) %ls\n", err, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	/* Print the application's error message. */
	err = errno;
	va_start(args, format);
	log_vprintf(level, format, args);
	va_end(args);

	/* Print the system error message. */
	printf(": (%d) %s\n", err, strerror(err));
#endif /* _WIN32 */
}

/**
 * Prints out logging information related to system errors that set the errno
 * variable whenever dealing with sockets. System error message is automatically
 * appended at the end.
 *
 * @param level  Severity of the logged information.
 * @param format Format of the desired output without the tag or system message.
 * @param ...    Additional variables to be populated.
 */
void log_sockerr(log_level_t level, const char *format, ...) {
	va_list args;
	int err;

#ifdef _WIN32
	LPTSTR szErrorMessage;

	/* Get the descriptive error message from the system. */
	err = WSAGetLastError();
	if (!FormatMessage(FORMAT_MESSAGE_FLAGS, NULL, err, FORMAT_MESSAGE_LANG,
					   (LPTSTR)&szErrorMessage, 0, NULL)) {
		szErrorMessage = strdup("FormatMessage failed");
	}

	/* Print the application's error message. */
	va_start(args, format);
	log_vprintf(level, format, args);
	va_end(args);

	/* Print the system error message. */
	printf(": WSAError (%d) %ls\n", err, szErrorMessage);

	/* Free up any resources. */
	LocalFree(szErrorMessage);
#else
	/* Print the application's error message. */
	err = errno;
	va_start(args, format);
	log_vprintf(level, format, args);
	va_end(args);

	/* Print the system error message. */
	printf(": (%d) %s\n", err, strerror(err));
#endif /* _WIN32 */
}
