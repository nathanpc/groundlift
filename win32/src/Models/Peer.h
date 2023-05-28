/**
 * Peer.h
 * GroundLift peer device on the network.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_MODELS_PEER_H
#define _WINGL_MODELS_PEER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <groundlift/client.h>

namespace GroundLift {

/**
 * GroundLift peer device on the network.
 */
class Peer {
private:
	LPTSTR szDeviceType;
	LPTSTR szHostname;
	struct sockaddr *saAddress;
	socklen_t lenAddress;

public:
	Peer(LPCTSTR szDeviceType, LPCTSTR szHostname,
		 const struct sockaddr *saAddress, socklen_t lenAddress);
	Peer(LPCTSTR szDeviceType, LPCTSTR szHostname, LPCTSTR szIPAddress,
		 USHORT usPort);
	Peer();
	virtual ~Peer();

	LPCTSTR DeviceType();
	LPCTSTR Hostname();
	const struct sockaddr *SocketAddress();
	socklen_t SocketAddressLength();
	LPTSTR IPAddress();

protected:
	void Initialize(LPCTSTR szDeviceType, LPCTSTR szHostname,
		 const struct sockaddr *saAddress, socklen_t lenAddress);

};

}

#endif // _WINGL_MODELS_PEER_H
