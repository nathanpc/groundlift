/**
 * server.h
 * GroundLift command-line server.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLI_SERVER_H
#define _GL_CLI_SERVER_H

#include <groundlift/server.h>

#ifdef __cplusplus
extern "C" {
#endif

gl_err_t *server_run(const char *ip, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLI_SERVER_H */
