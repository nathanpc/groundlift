/**
 * Controllers/Client.cpp
 * GroundLift client controller.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Client.h"

#include <stdexcept>
#include <groundlift/defaults.h>
#include <utils/logging.h>
#include <utils/utf16.h>

#include "../Exceptions/GroundLift.h"

using namespace GroundLift;

/**
 * Constructs a new object.
 */
Client::Client() {
	this->hndClient = gl_client_new();
	if (this->hndClient == NULL) {
		throw GroundLift::Exception(
			gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
				EMSG("Failed to construct the client handle object"))
		);
	}
}

/**
 * Frees up any resources allocated internally for the object.
 */
Client::~Client() {
	// Wait for the client's thread to return.
	gl_err_t *err = gl_client_thread_join(this->hndClient);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Client thread returned with errors.\n");
		gl_error_free(err);
	}

	// Free our client handle.
	gl_client_free(this->hndClient);
	this->hndClient = NULL;
}

/**
 * Sends a file to another peer.
 * 
 * @param szDest IP address of the peer.
 * @param usPort GroundLift's server port of the peer.
 * @param szPath Path of the file to be sent.
 */
void Client::SendFile(LPCTSTR szDest, USHORT usPort, LPCTSTR szPath) {
	gl_err_t *err = NULL;
	char *szmbDest;
	char *szmbPath;

	// Convert our parameters to UTF-8.
	szmbDest = utf16_wcstombs(szDest);
	szmbPath = utf16_wcstombs(szPath);

	// Setup the client.
	err = gl_client_setup(this->hndClient, szmbDest, usPort, szmbPath);
	if (err)
		throw GroundLift::Exception(err);

	// Connect to the server.
	err = gl_client_connect(this->hndClient);
	if (err)
		throw GroundLift::Exception(err);

	// Free our temporary buffers.
	free(szmbDest);
	free(szmbPath);
}

/**
 * Sets the Connection Request Reply event handler.
 * 
 * @param func Connection Request Reply event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Client::SetConnectionResponseEvent(
		gl_client_evt_conn_req_resp_func func, void *arg) {
	gl_client_evt_conn_req_resp_set(this->hndClient, func, arg);
}

/**
 * Sets the Send File Operation Progress event handler.
 * 
 * @param func Send File Operation Succeeded event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Client::SetProgressEvent(gl_client_evt_put_progress_func func,
							  void *arg) {
	gl_client_evt_put_progress_set(this->hndClient, func, arg);
}

/**
 * Sets the Send File Operation Succeeded event callback function.
 * 
 * @param func Send File Operation Succeeded event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Client::SetSuccessEvent(gl_client_evt_put_succeed_func func, void *arg) {
	gl_client_evt_put_succeed_set(this->hndClient, func, arg);
}

/**
 * Gets the client handle structure.
 * 
 * @return Client's handle structure.
 */
client_handle_t* Client::Handle() {
	return this->hndClient;
}
