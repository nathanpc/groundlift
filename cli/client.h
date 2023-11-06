/**
 * client.h
 * GroundLift command-line client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_CLI_CLIENT_H
#define _GL_CLI_CLIENT_H

#include <groundlift/client.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public handles. */
extern client_handle_t *g_client;

/* Operations */
/*gl_err_t *client_send(const char *ip, uint16_t port, const char *fname);*/
gl_err_t *client_list_peers(const struct sockaddr *sock_addr, bool verbose);
gl_err_t *client_list_peers_ifs(void);

#ifdef __cplusplus
}
#endif

#endif /* _GL_CLI_CLIENT_H */
