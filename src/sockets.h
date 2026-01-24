/**
 * sockets.h
 * Platform-independent abstraction layer over socket operations.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_SOCKETS_H
#define _GL_SOCKETS_H

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <wspiapi.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
#endif /* _WIN32 */

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
	#define SOCKERR   SOCKET_ERROR
	#define sockclose closesocket
	#define sockerrno WSAGetLastError()
	typedef SOCKET sockfd_t;

	#ifndef EWOULDBLOCK
		#define EWOULDBLOCK WSAEWOULDBLOCK
	#endif /* !EWOULDBLOCK */
#else
	#define SOCKERR   (-1)
	#define sockclose close
	#define sockerrno errno
	typedef int sockfd_t;
#endif /* _WIN32 */

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

/* Ensure we have a definition for the length of an IPv6 string. */
#ifndef INET6_ADDRSTRLEN
	#define INET6_ADDRSTRLEN 46
#endif /* !INET6_ADDRSTRLEN */

/* Ensure we are able to store an IP address inside a string. */
#define IPADDR_STRLEN INET6_ADDRSTRLEN

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization */
bool socket_init(void);

/* Socket addressing operations. */
bool socket_addr_setup(struct sockaddr_storage *sa, int *af, socklen_t *addrlen,
                       const char *addr, const char *port);

/* Server and client. */
sockfd_t socket_new_server(const char *addr, const char *port);
sockfd_t socket_new_client(const char *addr, const char *port);

/* Utilities */
int socket_close(sockfd_t sockfd, bool shut);
const char* inet_addr_str(int af, void *addr, char *buf);

#ifdef __cplusplus
}
#endif

#endif /*_GL_SOCKETS_H */
