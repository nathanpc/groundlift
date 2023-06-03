/**
 * Controllers/Server.cpp
 * GroundLift server controller.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Server.h"

#include <stdexcept>
#include <groundlift/defaults.h>
#include <utils/logging.h>

#include "../Exceptions/GroundLift.h"

using namespace GroundLift;

/**
 * Constructs a new object.
 */
Server::Server() {
	// Create a brand new server handle.
	this->hndServer = gl_server_new();
	if (this->hndServer == NULL) {
		throw GroundLift::Exception(
			gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
				EMSG("Failed to construct the server handle object"))
		);
	}
}

/**
 * Frees up any resources allocated internally for the object.
 */
Server::~Server() {
	FreeHandle();
}

/**
 * Starts the server up and listens on the default port on all interfaces.
 */
void Server::Start() {
	gl_err_t *err;

	// Are we coming back for a restart?
	if (!IsRunning())
		Setup();

	// Start the main server it up!
	err = gl_server_start(this->hndServer);
	if (err)
		throw GroundLift::Exception(err);

	// Start the discovery server.
	err = gl_server_discovery_start(this->hndServer, UDPSERVER_PORT);
	if (err)
		throw GroundLift::Exception(err);
}

/**
 * Stop the server from listening to incoming requests.
 */
void Server::Stop() {
	// Stop the server abruptly.
	gl_server_stop(this->hndServer);
}

/**
 * Restarts the server.
 */
void Server::Restart() {
	Stop();
	Start();
}

/**
 * Stops the server and frees its handle and any associated resources.
 */
void Server::FreeHandle() {
	gl_err_t *err;

	// Stop the server just in case.
	Stop();

	// Wait for the discovery server thread to return.
	err = gl_server_discovery_thread_join(this->hndServer);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR,
				   "Discovery server thread returned with errors.\n");
		gl_error_free(err);
	}

	// Wait for the main server's thread to return.
	err = gl_server_thread_join(this->hndServer);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Server thread returned with errors.\n");
		gl_error_free(err);
	}

	// Free our server handle.
	gl_server_free(this->hndServer);
	this->hndServer = NULL;
}

/**
 * Sets up a brand new server handle.
 */
void Server::Setup() {
	gl_err_t *err = gl_server_setup(this->hndServer, NULL, TCPSERVER_PORT);
	if (err)
		throw GroundLift::Exception(err);
}

/**
 * Checks if the server is currently running.
 * 
 * @return Is the server running?
 */
bool Server::IsRunning() {
	if ((this->hndServer == NULL) || (this->hndServer->server == NULL))
		return false;

	return this->hndServer->server->tcp.sockfd != INVALID_SOCKET;
}

/**
 * Sets the Server Started event callback function.
 * 
 * @param func Server Started event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetStartEvent(gl_server_evt_start_func func, void* arg) {
	gl_server_evt_start_set(this->hndServer, func, arg);
}

/**
 * Sets the Server Stopped event callback function.
 *
 * @param func Server Stopped event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetStopEvent(gl_server_evt_stop_func func, void *arg) {
	gl_server_evt_stop_set(this->hndServer, func, arg);
}

/**
 * Sets the Client Socket Connection Accepted event callback function.
 *
 * @param func Client Socket Connection Accepted event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetConnectionAcceptedEvent(gl_server_conn_evt_accept_func func,
										void *arg) {
	gl_server_conn_evt_accept_set(this->hndServer, func, arg);
}

/**
 * Sets the Client Socket Connection Closed event callback function.
 *
 * @param func Client Socket Connection Closed event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetConnectionCloseEvent(gl_server_conn_evt_close_func func,
									 void *arg) {
	gl_server_conn_evt_close_set(this->hndServer, func, arg);
}

/**
 * Sets the Client Transfer Requested event callback function.
 *
 * @param func Client Transfer Requested event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetClientTransferRequestEvent(
		gl_server_evt_client_conn_req_func func, void *arg) {
	gl_server_evt_client_conn_req_set(this->hndServer, func, arg);
}

/**
 * Sets the File Download Progress event callback function.
 *
 * @param func File Download Progress event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetDownloadProgressEvent(
		gl_server_conn_evt_download_progress_func func, void *arg) {
	gl_server_conn_evt_download_progress_set(this->hndServer, func, arg);
}

/**
 * Sets the File Downloaded Successfully event callback function.
 *
 * @param func File Downloaded Successfully event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void Server::SetDownloadSuccessEvent(
		gl_server_conn_evt_download_success_func func, void *arg) {
	gl_server_conn_evt_download_success_set(this->hndServer, func, arg);
}

/**
 * Gets the server handle structure.
 * 
 * @return Server's handle structure.
 */
server_handle_t* Server::Handle() {
	return this->hndServer;
}
