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
	#define GL_SERVER_PORT "1650"
#endif /* GL_SERVER_PORT */

/**
 * Copyright text.
 */
#ifndef GL_COPYRIGHT
	#define GL_COPYRIGHT "Copyright (C) 2023-2026 Nathan Campos"
#endif /* GL_COPYRIGHT */

/**
 * Server's file receive buffer length.
 */
#ifndef RECV_BUF_LEN
	#define RECV_BUF_LEN 1024
#endif /* RECV_BUF_LEN */

/**
 * Client's file send buffer length.
 */
#ifndef SEND_BUF_LEN
	#define SEND_BUF_LEN RECV_BUF_LEN
#endif /* SEND_BUF_LEN */

/**
 * Receive request line's maximum length.
 */
#ifndef GL_REQLINE_MAX
	#define GL_REQLINE_MAX 400
#endif /* GL_REQLINE_MAX */

/**
 * Server reply line's maximum length.
 */
#ifndef GL_REPLYLINE_MAX
	#define GL_REPLYLINE_MAX GL_REQLINE_MAX
#endif /* GL_REPLYLINE_MAX */

#endif /* _GL_DEFAULTS_H */
