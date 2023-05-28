/**
 * Controllers/PeerDiscovery.h
 * GroundLift peer discovery service controller.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "PeerDiscovery.h"

#include <stdexcept>
#include <groundlift/defaults.h>
#include <utils/logging.h>
#include <utils/utf16.h>

#include "../Exceptions/GroundLift.h"

using namespace GroundLift;

/**
 * WARNING REGARDING VISUAL C++ 6:
 *
 * When compiling this class using Visual Studio 6.0 you'll need to patch an
 * SDK header in order to get rid of some compilation issues due to the nature
 * of the project that mixes a C library with C++.
 *
 * Consult "docs/compiling_mscv6.md" for instructions.
 */

/**
 * Constructs a new object.
 */
PeerDiscovery::PeerDiscovery() {
	this->hndClient = gl_client_discovery_new();
	if (this->hndClient == NULL) {
		throw GroundLift::Exception(
			gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
				EMSG("Failed to construct the discovery client handle object"))
		);
	}
}

/**
 * Frees up any resources allocated internally for the object.
 */
PeerDiscovery::~PeerDiscovery() {
	// Wait for the discovery thread to return.
	gl_err_t *err = gl_client_discovery_thread_join(this->hndClient);
	if (err) {
		gl_error_print(err);
		log_printf(LOG_ERROR, "Peer discovery thread returned with errors.\n");

		gl_error_free(err);
	}

	// Free our discovery handle.
	gl_client_discovery_free(this->hndClient);
	this->hndClient = NULL;
}

/**
 * Scans the network for peers.
 * 
 * @see SetPeerDiscoveredEvent
 */
void PeerDiscovery::Scan() {
	gl_err_t *err = NULL;

	// Setup the peer discovery service socket.
	err = gl_client_discovery_setup(this->hndClient, UDPSERVER_PORT);
	if (err)
		throw Exception(err);

	// Send discovery broadcast and listen to replies.
	err = gl_client_discover_peers(this->hndClient);
	if (err)
		throw Exception(err);
}

/**
 * Sets the Peer Discovered event handler.
 *
 * @param func Peer Discovered event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void PeerDiscovery::SetPeerDiscoveredEvent(
		gl_client_evt_discovery_peer_func func, void *arg) {
	gl_client_evt_discovery_peer_set(this->hndClient, func, arg);
}

/**
 * Gets the peer discovery handle structure.
 * 
 * @return Peer discovery handle structure.
 */
discovery_client_t *PeerDiscovery::Handle() {
	return this->hndClient;
}
