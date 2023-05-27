/**
 * Controllers/PeerDiscovery.h
 * GroundLift peer discovery service controller.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_CONTROLLERS_PEERDISCOVERY_H
#define _WINGL_CONTROLLERS_PEERDISCOVERY_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <groundlift/client.h>

namespace GroundLift {

/**
 * GroundLift peer discovery service controller.
 */
class PeerDiscovery {
private:
	discovery_client_t *hndClient;

public:
	PeerDiscovery();
	virtual ~PeerDiscovery();

	void Scan();
	void SetPeerDiscoveredEvent(gl_client_evt_discovery_peer_func func,
								void *arg);

	discovery_client_t *Handle();
};

}

#endif // _WINGL_CONTROLLERS_PEERDISCOVERY_H
