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
#define TCPSERVER_PORT 1234
#endif /* TCPSERVER_PORT */

/* Number of connections in the TCP server queue. */
#ifndef TCPSERVER_BACKLOG
#define TCPSERVER_BACKLOG 10
#endif /* TCPSERVER_BACKLOG */

#ifdef __cplusplus
}
#endif

#endif /* _PROJECT_DEFAULTS_H */