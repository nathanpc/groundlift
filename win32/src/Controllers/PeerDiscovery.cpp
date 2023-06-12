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
	// Allocate a new discovery client.
	this->hndClient = gl_client_discovery_new();
	if (this->hndClient == NULL) {
		throw GroundLift::Exception(
			gl_error_new_errno(ERR_TYPE_GL, GL_ERR_UNKNOWN,
				EMSG("Failed to construct the discovery client handle object"))
		);
	}

	// Setup the finished flag.
	this->bFinished = false;
	SetDiscoveredFinishedEvent(OnFinished, this);
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
 * Scans the network for peers using a classic broadcast to the INADDR_BROADCAST
 * address.
 * 
 * @see SetPeerDiscoveredEvent
 */
void PeerDiscovery::Scan() {
	Scan(NULL);
}

/**
 * Scans the network for peers using a specific broadcast address.
 * 
 * @param sock_addr Broadcast IP address to connect/bind to or NULL if the
 *                  address should be INADDR_BROADCAST.
 *
 * @see SetPeerDiscoveredEvent
 */
void PeerDiscovery::Scan(const struct sockaddr* sock_addr) {
	gl_err_t *err = NULL;

	// Setup the peer discovery service socket.
	if (sock_addr == NULL) {
		// Use the 255.255.255.255 IP address for broadcasting.
		err = gl_client_discovery_setup(this->hndClient, INADDR_BROADCAST,
										UDPSERVER_PORT);
	} else {
		// Use a specific broadcast IP address for a specific network.
		switch (sock_addr->sa_family) {
			case AF_INET:
				err = gl_client_discovery_setup(this->hndClient,
					reinterpret_cast<const struct sockaddr_in *>(sock_addr)
						->sin_addr.s_addr, UDPSERVER_PORT);
				break;
			case AF_INET6:
			default:
				err = gl_error_new(ERR_TYPE_GL, GL_ERR_UNKNOWN,
								   EMSG("Discovery service only works with IPv4"));
				break;
		}
	}

	// Check for errors.
	if (err)
		throw Exception(err);

	// Send discovery broadcast and listen to replies.
	err = gl_client_discover_peers(this->hndClient);
	if (err)
		throw Exception(err);
}

#ifndef SINGLE_IFACE_MODE
/**
 * Scans all networks for peers throughout all of the network interfaces.
 *
 * @param func Peer Discovered event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 * 
 * @return A list with all the used peer discovery objects for disposal.
 */
std::vector<PeerDiscovery *>* PeerDiscovery::ScanAllNetworks(
		gl_client_evt_discovery_peer_func func, void* arg) {
	std::vector<PeerDiscovery *> *vecDiscovery;
	iface_info_list_t *iil;

	// Get the list of network interfaces.
	tcp_err_t err = socket_iface_info_list(&iil);
	if (err) {
		throw GroundLift::Exception(
			gl_error_new_errno(ERR_TYPE_TCP, (int8_t)err,
				EMSG("Failed to get the list of network interfaces"))
		);
	}

	// Go through the network interfaces.
	vecDiscovery = new std::vector<PeerDiscovery *>();
	for (int i = 0; i < iil->count; ++i) {
		// Get the network interface and create a new object of us.
		iface_info_t *iface = iil->ifaces[i];
		PeerDiscovery *discovery = new PeerDiscovery();

		// Setup the event handler and search for peers on this network.
		discovery->SetPeerDiscoveredEvent(func, arg);
		discovery->Scan(iface->brdaddr);

		// Append our discovery object to the list.
		vecDiscovery->push_back(discovery);
	}

	// Free our network interface list.
	socket_iface_info_list_free(iil);

	return vecDiscovery;
}

/**
 * Handles the proper destruction of a PeerDiscovery object when discovering
 * peers on multiple network interfaces.
 *
 * @param arg Pointer to our own PeerDiscovery object to be marked as finished.
 */
void PeerDiscovery::OnFinished(void *arg) {
	PeerDiscovery *discovery = reinterpret_cast<PeerDiscovery *>(arg);
	discovery->bFinished = true;
}
#endif // !SINGLE_IFACE_MODE

/**
 * Sets the Peer Discovered event handler.
 *
 * @param func Peer Discovered event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void PeerDiscovery::SetPeerDiscoveredEvent(
		gl_client_evt_discovery_peer_func func, void* arg) {
	gl_client_evt_discovery_peer_set(this->hndClient, func, arg);
}

/**
 * Sets the Peer Discovery Finished event handler.
 *
 * @param func Peer Discovery Finished event callback function.
 * @param arg  Optional parameter to be sent to the event handler.
 */
void PeerDiscovery::SetDiscoveredFinishedEvent(
		gl_client_evt_discovery_end_func func, void* arg) {
	gl_client_evt_discovery_end_set(this->hndClient, func, arg);
}

/**
 * Gets the peer discovery handle structure.
 * 
 * @return Peer discovery handle structure.
 */
discovery_client_t *PeerDiscovery::Handle() {
	return this->hndClient;
}
