/**
 * Controllers/Client.h
 * GroundLift client controller.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_CONTROLLERS_CLIENT_H
#define _WINGL_CONTROLLERS_CLIENT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <groundlift/client.h>

namespace GroundLift {

/**
 * GroundLift client controller.
 */
class Client {
private:
	client_handle_t *hndClient;

public:
	Client();
	virtual ~Client();

	void SendFile(LPCTSTR szDest, USHORT usPort, LPCTSTR szPath);

	// Event handlers.
	void SetConnectionResponseEvent(gl_client_evt_conn_req_resp_func func,
									void *arg);
	void SetProgressEvent(gl_client_evt_put_progress_func func, void *arg);
	void SetSuccessEvent(gl_client_evt_put_succeed_func func, void *arg);

	client_handle_t *Handle();
};

}

#endif // _WINGL_CONTROLLERS_CLIENT_H
