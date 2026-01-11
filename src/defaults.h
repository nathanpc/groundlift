/**
 * defaults.h
 * Some default values that may be used throughout the entire project.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _GL_DEFAULTS_H
#define _GL_DEFAULTS_H

/**
 * Default port.
 */
#ifndef GL_SERVER_PORT
	#define GL_SERVER_PORT 1650
#endif /* GL_SERVER_PORT */

/**
 * Server's file receive buffer length.
 */
#ifndef RECV_BUF_LEN
	#define RECV_BUF_LEN 1024
#endif /* RECV_BUF_LEN */

/**
 * Receive request line's maximum length.
 */
#ifndef GL_REQLINE_MAX
	#define GL_REQLINE_MAX 400
#endif /* GL_REQLINE_MAX */

#endif /* _GL_DEFAULTS_H */
