/**
 * SendFileDialog.h
 * A dialog to send files to a specific client.
 * 
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINGL_SENDFILEDIALOG_H
#define _WINGL_SENDFILEDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <DialogWindow.h>
#include <vector>

#include "Models/Peer.h"
#include "Controllers/PeerDiscovery.h"

/**
 * A dialog to send files to a specific client.
 */
class SendFileDialog : public DialogWindow {
protected:
	HWND hwndAddressEdit;
	HWND hwndPeerList;
	HWND hwndFilePathEdit;

	GroundLift::PeerDiscovery peerDiscovery;
	std::vector<GroundLift::Peer *> vecPeers;

public:
	SendFileDialog(HINSTANCE& hInst, HWND& hwndParent);
	virtual ~SendFileDialog();

	void RefreshPeerList();
	void AppendPeerToList(GroundLift::Peer *peer);

private:
	const int kDeviceTypeColIndex = 0;
	const int kHostnameColIndex = 1;
	const int kIPAddressColIndex = 2;

	void SetupPeerList();
	void AddPeerToList(int iRow, GroundLift::Peer *peer);
	void AddPeerToList(int iRow, LPCTSTR szDeviceType, LPCTSTR szHostname,
					   LPCTSTR szIPAddress);
	void ClearPeersVector();

	INT_PTR ListClientsOnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);
	INT_PTR ButtonBrowseOnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

	static void PeerDiscoveryEventHandler(const gl_discovery_peer_t *peer,
										  void *arg);

	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);
};

#endif // _WINGL_SENDFILEDIALOG_H
