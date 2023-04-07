/**
 * defaults.h
 * A collection of default definitions for various aspects of the program that
 * can be overwritten during the compilation phase.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _PROJECT_DEFAULTS_H
#define _PROJECT_DEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Server port to listen on. */
#ifndef TCPSERVER_PORT
#define TCPSERVER_PORT 1650
#endif /* TCPSERVER_PORT */

/* Number of connections in the TCP server queue. */
#ifndef TCPSERVER_BACKLOG
#define TCPSERVER_BACKLOG 10
#endif /* TCPSERVER_BACKLOG */

/* Ensure we know the size of a wchar_t in this platform. */
#ifndef _SIZEOF_WCHAR
	#ifdef _WIN32
		#define _SIZEOF_WCHAR 2
	#elif defined(__SIZEOF_WCHAR_T__)
		#define _SIZEOF_WCHAR __SIZEOF_WCHAR_T__
	#else
		#error _SIZEOF_WCHAR was not defined. Use the script under \
		scripts/wchar-size.sh to determine its size and define the macro in your \
		build system.
	#endif
#endif /* _SIZEOF_WCHAR */

#ifdef __cplusplus
}
#endif

#endif /* _PROJECT_DEFAULTS_H */