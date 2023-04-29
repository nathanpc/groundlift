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

/* Discovery port to listen on. */
#ifndef UDPSERVER_PORT
#define UDPSERVER_PORT 1651
#endif /* UDPSERVER_PORT */

/* Number of connections in the TCP server queue. */
#ifndef TCPSERVER_BACKLOG
#define TCPSERVER_BACKLOG 10
#endif /* TCPSERVER_BACKLOG */

/* Timeout of UDP communications in milliseconds. */
#ifndef UDP_TIMEOUT_MS
#define UDP_TIMEOUT_MS 5000
#endif /* UDP_TIMEOUT_MS */

/* OBEX protocol version to be disclosed to clients. */
#ifndef OBEX_PROTO_VERSION
#define OBEX_PROTO_VERSION 0x10
#endif /* OBEX_PROTO_VERSION */

/* Maximum size of an OBEX packet in bytes supported by the application. */
#ifndef OBEX_MAX_PACKET_SIZE
#define OBEX_MAX_PACKET_SIZE 65535
#endif /* OBEX_MAX_PACKET_SIZE */

/* Maximum size of a file chunk in an OBEX PUT packet in bytes. */
#ifndef OBEX_MAX_FILE_CHUNK
#define OBEX_MAX_FILE_CHUNK 8000
#endif /* OBEX_MAX_FILE_CHUNK */

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

/* Ensure we know the maximum length that the machine's hostname can be. */
#ifndef HOST_NAME_MAX
	#ifdef _WIN32
		#define HOST_NAME_MAX MAX_COMPUTERNAME_LENGTH
	#elif defined(MAXHOSTNAMELEN)
		#define HOST_NAME_MAX MAXHOSTNAMELEN
	#else
		#define HOST_NAME_MAX 64
	#endif /* _WIN32 */
#endif /* HOST_NAME_MAX */

/* Character used for separating paths in the current environment. */
#ifndef PATH_SEPARATOR
	#ifdef _WIN32
		#define PATH_SEPARATOR '\\'
	#else
		#define PATH_SEPARATOR '/'
	#endif
#endif /* PATH_SEPARATOR */

#ifdef __cplusplus
}
#endif

#endif /* _PROJECT_DEFAULTS_H */
