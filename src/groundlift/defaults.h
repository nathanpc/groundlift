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

/* Main server port to listen on. */
#ifndef GL_SERVER_MAIN_PORT
#define GL_SERVER_MAIN_PORT 1650
#endif /* GL_SERVER_MAIN_PORT */

/* Peer discovery service port to listen back for replies on. */
#ifndef GL_CLIENT_DISCOVERY_PORT
#define GL_CLIENT_DISCOVERY_PORT 1651
#endif /* GL_CLIENT_DISCOVERY_PORT */

/* Timeout of UDP communications in milliseconds. */
#ifndef UDP_TIMEOUT_MS
#define UDP_TIMEOUT_MS 1000
#endif /* UDP_TIMEOUT_MS */

/* Glproto device type. */
#ifndef GL_DEVICE_TYPE
#define GL_DEVICE_TYPE "Ukn"
#endif /* GL_DEVICE_TYPE */

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

#ifdef __cplusplus
}
#endif

#endif /* _PROJECT_DEFAULTS_H */
