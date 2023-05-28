/**
 * Peer.cpp
 * GroundLift peer device on the network.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Peer.h"

#include <stdexcept>
#include <utils/utf16.h>
#include <groundlift/sockets.h>

using namespace GroundLift;

/**
 * Constructs a new Peer object.
 * 
 * @param szDeviceType Type of device of the peer.
 * @param szHostname   Hostname of the peer.
 * @param saAddress    Socket address structure pointing to the peer.
 * @param lenAddress   Length in bytes of the socket address structure.
 */
Peer::Peer(LPCTSTR szDeviceType, LPCTSTR szHostname,
		   const struct sockaddr *saAddress, socklen_t lenAddress) {
	Initialize(szDeviceType, szHostname, saAddress, lenAddress);
}

/**
 * Constructs a new Peer object.
 * 
 * @param szDeviceType Type of device of the peer.
 * @param szHostname   Hostname of the peer.
 * @param szIPAddress  IP address of the peer.
 * @param usPort       Port of the server on the peer.
 */
Peer::Peer(LPCTSTR szDeviceType, LPCTSTR szHostname, LPCTSTR szIPAddress,
		   USHORT usPort) {
	struct sockaddr_in *addr;
	char *szIP;

	// Initialize common properties.
	Initialize(szDeviceType, szHostname, NULL, 0);

	// Convert our IP to UTF-8.
	szIP = utf16_wcstombs(szIPAddress);
	if (szIP == NULL)
		throw std::exception("Failed to convert IP address to UTF-8");

	// Allocate our socket address structure.
	this->lenAddress = sizeof(struct sockaddr_in);
	addr = static_cast<struct sockaddr_in *>(malloc(this->lenAddress));
	if (addr == NULL)
		throw std::bad_alloc();
	ZeroMemory(addr, this->lenAddress);

	// Populate our socket address structure.
	addr->sin_family = AF_INET;
	addr->sin_port = htons(usPort);
	addr->sin_addr.S_un.S_addr = socket_inet_addr(szIP);
	this->saAddress = reinterpret_cast<struct sockaddr *>(addr);

	// Free our temporary resources.
	free(szIP);
}

/**
 * Constructs an empty Peer object.
 */
Peer::Peer() {
	this->szDeviceType = NULL;
	this->szHostname = NULL;
	this->saAddress = NULL;
	this->lenAddress = 0;
}

/**
 * Frees up any resources allocated internally for the object.
 */
Peer::~Peer() {
	// Free our strings.
	if (this->szDeviceType)
		free(this->szDeviceType);
	if (this->szHostname)
		free(this->szHostname);

	// Free the allocated socket address structure if needed.
	if (this->saAddress)
		free(this->saAddress);
}

/**
 * Gets the type of device of the peer.
 * 
 * @return Type of device of the peer.
 */
LPCTSTR Peer::DeviceType() {
	return this->szDeviceType;
}

/**
 * Gets the hostname of the peer.
 * 
 * @return Hostname of the peer.
 */
LPCTSTR Peer::Hostname() {
	return this->szHostname;
}

/**
 * Gets the socket address structure associated with the peer.
 * 
 * @return Socket address structure.
 */
const struct sockaddr* Peer::SocketAddress() {
	return this->saAddress;
}

/**
 * Gets the length of the socket address structure associated with the peer.
 * 
 * @return Socket address structure length in bytes.
 */
socklen_t Peer::SocketAddressLength() {
	return this->lenAddress;
}

/**
 * Gets the IP address of the peer as a string.
 * @warning This function allocates memory that must be free'd by you!
 * 
 * @return Newly allocated IP address of the peer.
 */
LPTSTR Peer::IPAddress() {
	char *szBuffer;
	WCHAR *wszBuffer;
	
	// Ensure we have a valid socket structure.
	if (this->saAddress == NULL)
		return NULL;

	// Get the IP address as a string.
	if (!socket_itos(&szBuffer, this->saAddress))
		return NULL;

	// Convert the string to UTF-16 and free our temporary buffer
	wszBuffer = utf16_mbstowcs(szBuffer);
	free(szBuffer);

	return wszBuffer;
}

/**
 * Initializes some of the object's properties. This is used to provide a
 * common initialization section for our constructors.
 * 
 * @param szDeviceType Type of device of the peer.
 * @param szHostname   Hostname of the peer.
 * @param saAddress    Socket address structure pointing to the peer.
 * @param lenAddress   Length in bytes of the socket address structure.
 */
void Peer::Initialize(LPCTSTR szDeviceType, LPCTSTR szHostname,
					  const struct sockaddr *saAddress, socklen_t lenAddress) {
	this->szDeviceType = _tcsdup(szDeviceType);
	this->szHostname = _tcsdup(szHostname);
	this->saAddress = NULL;
	this->lenAddress = lenAddress;

	// Allocate memory and copy our socket address structure.
	if (saAddress) {
		this->saAddress = static_cast<struct sockaddr *>(malloc(lenAddress));
		if (this->saAddress == NULL)
			throw std::bad_alloc();
		memcpy(this->saAddress, saAddress, lenAddress);
	}
}
