/**
 * Controllers/Server.h
 * GroundLift server controller.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_CONTROLLERS_SERVER_H
#define _WINGL_CONTROLLERS_SERVER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <groundlift/server.h>

namespace GroundLift {

/**
 * GroundLift server controller.
 */
class Server {
private:
	server_handle_t *hndServer;

	void Setup();
	void FreeHandle();

public:
	Server();
	virtual ~Server();

	void Start();
	void Stop();
	void Restart();
	bool IsRunning();

	// Event handlers.
	void SetStartEvent(gl_server_evt_start_func func, void *arg);
	void SetStopEvent(gl_server_evt_stop_func func, void *arg);
	void SetConnectionAcceptedEvent(gl_server_conn_evt_accept_func func,
									void *arg);
	void SetConnectionCloseEvent(gl_server_conn_evt_close_func func,
								 void *arg);
	void SetClientTransferRequestEvent(
		gl_server_evt_client_conn_req_func func, void *arg);
	void SetDownloadProgressEvent(
		gl_server_conn_evt_download_progress_func func, void *arg);
	void SetDownloadSuccessEvent(
		gl_server_conn_evt_download_success_func func, void *arg);

	server_handle_t* Handle();
};

}

#endif // _WINGL_CONTROLLERS_SERVER_H
