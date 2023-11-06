/**
 * sockets.h
 * Platform-independent abstraction layer over the sockets API.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_SOCKETS_H
#define _GL_SOCKETS_H

#ifdef _WIN32
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include "../common/win32/stdshim.h"
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
#endif /* _WIN32 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	/* Shim things that Microsoft forgot to include in their implementation. */
	#ifndef in_addr_t
		typedef unsigned long in_addr_t;
	#endif /* in_addr_t */
#endif /* _WIN32 */

/* Cross-platform socket function return error code. */
#ifndef SOCKET_ERROR
	#define SOCKET_ERROR (-1)
#endif /* !SOCKET_ERROR */

/* Cross-platform representation of an invalid socket file descriptor. */
#ifndef INVALID_SOCKET
	#ifdef _WIN32
		#define INVALID_SOCKET (SOCKET)(~0)
	#else
		#define INVALID_SOCKET (-1)
	#endif /* _WIN32 */
#endif /* !INVALID_SOCKET */

/* Cross-platform socket file descriptor. */
#ifndef SOCKFD
	#ifdef _WIN32
		#define SOCKFD SOCKET
	#else
		#define SOCKFD int
	#endif /* _WIN32 */
#endif /* !SOCKFD */

/* Cross-platform shim for socket error codes. */
#ifdef _WIN32
	#define sockerrno WSAGetLastError()
#else
	#define sockerrno errno
#endif /* _WIN32 */

/* Socket handle. */
typedef struct {
	SOCKFD fd;

	struct sockaddr *addr;
	socklen_t addr_len;
} sock_handle_t;

#ifndef SINGLE_IFACE_MODE
/* Network interface information object. */
typedef struct {
	char *name;
	struct sockaddr *ifaddr;
	struct sockaddr *brdaddr;
} iface_info_t;

/* Network interface information object list. */
typedef struct {
	iface_info_t **ifaces;
	uint8_t count;
} iface_info_list_t;
#endif /* !SINGLE_IFACE_MODE */

/* Initialization and destruction. */
sock_handle_t *socket_new(void);
sock_handle_t *socket_dup(const sock_handle_t *sock);
void socket_free(sock_handle_t *sock);
void socket_setaddr(sock_handle_t *sock, const char *addr, uint16_t port);
void socket_setaddr_inaddr(sock_handle_t *sock, in_addr_t inaddr,
						   uint16_t port);

/* Socket setup. */
gl_err_t *socket_setup_tcp(sock_handle_t *sock, bool server);
gl_err_t *socket_setup_udp(sock_handle_t *sock, bool server,
						   uint32_t timeout_ms);

/* Socket operations. */
sock_handle_t *socket_accept(sock_handle_t *server);
gl_err_t *socket_connect(sock_handle_t *sock);
gl_err_t *socket_send(const sock_handle_t *sock, const void *buf, size_t len,
					  size_t *sent_len);
gl_err_t *socket_sendto(const sock_handle_t *sock, const void *buf, size_t len,
						const struct sockaddr *sock_addr, socklen_t sock_len,
						size_t *sent_len);
gl_err_t *socket_recv(const sock_handle_t *sock, void *buf, size_t buf_len,
                      size_t *recv_len, bool peek, sock_err_t *serr);
gl_err_t *socket_recvfrom(const sock_handle_t *sock, void *buf, size_t buf_len,
                          struct sockaddr *sock_addr, socklen_t *sock_len,
                          size_t *recv_len, bool peek, sock_err_t *serr);
gl_err_t *socket_close(sock_handle_t *sock);
gl_err_t *socket_shutdown(sock_handle_t *sock);

/* Socket string conversions. */
bool socket_itos(char **buf, const struct sockaddr *sock_addr);
in_addr_t socket_inet_addr(const char *ipaddr);

#ifndef SINGLE_IFACE_MODE
/* Network interface management. */
iface_info_t *socket_iface_info_new(void);
gl_err_t *socket_iface_info_list(iface_info_list_t **if_list);
gl_err_t *socket_iface_info_list_push(iface_info_list_t *if_list,
									  iface_info_t *iface);
void socket_iface_info_list_free(iface_info_list_t *if_list);
void socket_iface_info_free(iface_info_t *iface);
#endif /* !SINGLE_IFACE_MODE */

#ifdef __cplusplus
}
#endif

#endif /* _GL_SOCKETS_H */
