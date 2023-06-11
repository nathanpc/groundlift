/**
 * SendFileDialog.cpp
 * A dialog to send files to a specific client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "SendFileDialog.h"

#include <utils/utf16.h>

#include "CommonIncludes.h"
#include "SendProgressDialog.h"

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
SendFileDialog::SendFileDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_SEND) {
	this->hwndAddressEdit = NULL;
	this->hwndPeerList = NULL;
	this->hwndFilePathEdit = NULL;

	// Setup the peer discovery event handler.
	this->peerDiscovery.SetPeerDiscoveredEvent(SendFileDialog::OnPeerDiscovered,
											   reinterpret_cast<void *>(this));
}

/**
 * Frees up any resources allocated by this object.
 */
SendFileDialog::~SendFileDialog() {
	ClearPeersVector();
}

/**
 * Refreshes the list of clients on the network using the peer discovery
 * broadcast system.
 */
void SendFileDialog::RefreshPeerList() {
	// Clear the list and vector in order to start fresh.
	ListView_DeleteAllItems(this->hwndPeerList);
	ClearPeersVector();

	// Start scanning around for peers.
#ifdef SINGLE_IFACE_MODE
	this->peerDiscovery.Scan();
#else
	this->peerDiscovery.ScanAllNetworks(SendFileDialog::OnPeerDiscovered,
										reinterpret_cast<void *>(this));
#endif	// SINGLE_IFACE_MODE
}

/**
 * Appends a peer to the list of peer and keeps track of its existance.
 * 
 * @param peer Peer to be added to the peer list.
 */
void SendFileDialog::AppendPeerToList(GroundLift::Peer *peer) {
	int iPos;

	// Append the peer to the vector and to the list control.
	this->vecPeers.push_back(peer);
	iPos = this->vecPeers.size() - 1;
	AddPeerToList(iPos, this->vecPeers.at(iPos));
}

/**
 * Sends the selected file to the selected peer.
 *
 * @param hDlg Dialog window handle.
 */
void SendFileDialog::SendFile(HWND hDlg) {
	SendProgressDialog *dlgProgress;
	LPTSTR szIP;
	LPTSTR szFilePath;

	// Get the necessary information from our dialog.
	szIP = GetWindowTextAlloc(this->hwndAddressEdit);
	szFilePath = GetWindowTextAlloc(this->hwndFilePathEdit);

	// TODO: Perform validation checks.

	// TODO: Check if the IP corresponds to a peer so that we can pass it over.
	
	// TODO: Make a factory so that we can properly dispose of this later.
	dlgProgress = new SendProgressDialog(this->hInst, this->hwndParent);
	dlgProgress->Show();

	// Send the file.
	dlgProgress->SendFile(szIP, szFilePath);

	// Free up our temporary buffers.
	free(szIP);
	free(szFilePath);
}

/**
 * Adds a peer to the list of peers.
 *
 * @param iRow Index of the row of the peer.
 * @param peer Peer to be added to the list control.
 */
void SendFileDialog::AddPeerToList(int iRow, GroundLift::Peer *peer) {
	LPTSTR szIP;

	// Add the peer to the list.
	szIP = peer->IPAddress();
	AddPeerToList(iRow, peer->DeviceType(), peer->Hostname(), szIP);

	// Free our temporary buffer.
	free(szIP);
}

/**
 * Adds a peer to the list of peers.
 * 
 * @param iRow         Index of the row of the peer.
 * @param szDeviceType String representation of the type of device of the peer.
 * @param szHostname   Hostname of the peer.
 * @param szIPAddress  IP address of the peer.
 */
void SendFileDialog::AddPeerToList(int iRow, LPCTSTR szDeviceType,
								   LPCTSTR szHostname, LPCTSTR szIPAddress) {
	LVITEM lvi;

	// Setup the common part of the item structure.
	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.iItem = iRow;
	lvi.stateMask = 0;
	lvi.state = 0;

	// Setup and add the Device Type cell.
	lvi.iSubItem = SendFileDialog::ColIndex::DeviceType;
	lvi.pszText = const_cast<LPTSTR>(szDeviceType);
	ListView_InsertItem(this->hwndPeerList, &lvi);

	// Setup and add the Hostname cell.
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = SendFileDialog::ColIndex::Hostname;
	lvi.pszText = const_cast<LPTSTR>(szHostname);
	ListView_SetItem(this->hwndPeerList, &lvi);

	// Setup and add the IP Address cell.
	lvi.iSubItem = SendFileDialog::ColIndex::IPAddress;
	lvi.pszText = const_cast<LPTSTR>(szIPAddress);
	ListView_SetItem(this->hwndPeerList, &lvi);
}

/**
 * Dialog window procedure.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 *
 * @see DefaultDlgProc
 */
INT_PTR CALLBACK SendFileDialog::DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
										 LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Get the handle of every useful control in the window.
			this->hwndAddressEdit = GetDlgItem(hDlg, IDC_EDIT_ADDRESS);
			this->hwndPeerList = GetDlgItem(hDlg, IDC_LIST_CLIENTS);
			this->hwndFilePathEdit = GetDlgItem(hDlg, IDC_EDIT_FILE);

			// Setup and refresh the peer list.
			SetupPeerList();
			RefreshPeerList();
			break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			if (cap_win_least_11()) {
				// Ensure we use the default background color of a window.
				return reinterpret_cast<INT_PTR>(
					GetSysColorBrush(COLOR_WINDOW));
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_BROWSE:
					return ButtonBrowseOnCommand(hDlg, wParam, lParam);
				case IDOK:
					SendFile(hDlg);
					break;
			}
			break;
		case WM_NOTIFY:
			LPNMHDR nmh = reinterpret_cast<LPNMHDR>(lParam);
			switch (nmh->idFrom) {
				case IDC_LIST_CLIENTS:
					return ListClientsOnNotify(hDlg, wParam, lParam);
			}
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Handles the WM_NOTIFY message of the client's List View.
 * 
 * @param hDlg   Dialog window handle.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 */
INT_PTR SendFileDialog::ListClientsOnNotify(HWND hDlg, WPARAM wParam,
											LPARAM lParam) {
	LPNMHDR nmh = reinterpret_cast<LPNMHDR>(lParam);

	switch (nmh->code) {
		case NM_CLICK:
			// Get the selected item.
			int iItem = ListView_GetNextItem(this->hwndPeerList, -1,
											 LVNI_SELECTED);
			if (iItem != -1) {
				// Set the IP address bar to the selected item's IP address.
				LPTSTR szIP = vecPeers[iItem]->IPAddress();
				SetWindowText(this->hwndAddressEdit, szIP);
				free(szIP);
			}
			break;
	}

	return FALSE;
}

/**
 * Handles the WM_COMMAND message of the Browse button.
 *
 * @param hDlg   Dialog window handle.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 */
INT_PTR SendFileDialog::ButtonBrowseOnCommand(HWND hDlg, WPARAM wParam,
											  LPARAM lParam) {
	OPENFILENAME ofn;
	TCHAR szPath[MAX_PATH] = _T("");

	// Populate the structure.
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrTitle = _T("Select File to Share");
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("All Files (*.*)\0*.*\0");
	ofn.lpstrFile = szPath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

	// Open the file dialog.
	if (!GetOpenFileName(&ofn))
		return FALSE;

	// Set the file path edit control's text.
	SetWindowText(this->hwndFilePathEdit, szPath);

	return TRUE;
}

/**
 * Sets up the peer list control with the proper columns and settings.
 */
void SendFileDialog::SetupPeerList() {
	LVCOLUMN lvc;

	// Setup the device type column.
	// TODO: Use an image here.
	ZeroMemory(&lvc, sizeof(LVCOLUMN));
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 30;
	lvc.pszText = const_cast<LPTSTR>(_T("Type"));
	lvc.iSubItem = SendFileDialog::ColIndex::DeviceType;
	ListView_InsertColumn(this->hwndPeerList, lvc.iSubItem, &lvc);

	// Setup the hostname column.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 140;
	lvc.pszText = const_cast<LPTSTR>(_T("Hostname"));
	lvc.iSubItem = SendFileDialog::ColIndex::Hostname;
	ListView_InsertColumn(this->hwndPeerList, lvc.iSubItem, &lvc);

	// Setup the IP address column.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 170;
	lvc.pszText = const_cast<LPTSTR>(_T("IP Address"));
	lvc.iSubItem = SendFileDialog::ColIndex::IPAddress;
	ListView_InsertColumn(this->hwndPeerList, lvc.iSubItem, &lvc);
}

/**
 * Clears the contents of the peer vector.
 */
void SendFileDialog::ClearPeersVector() {
	// Free up all of the members of the peer vector.
	for (std::vector<GroundLift::Peer*>::iterator i = this->vecPeers.begin();
		 i != vecPeers.end(); ++i) {
		delete *i;
	}

	// Clear the vector.
	this->vecPeers.clear();
}

/**
 * Handles the Peer Discovered event.
 * 
 * @param peer Discovered peer object.
 * @param arg  Optional data set by the event handler setup.
 */
void SendFileDialog::OnPeerDiscovered(const gl_discovery_peer_t *peer,
									  void *arg) {
	SendFileDialog *pThis;
	GroundLift::Peer *glPeer;
	LPTSTR szHostname;

	// Get ourselves from the optional argument.
	pThis = reinterpret_cast<SendFileDialog *>(arg);
	if (pThis == NULL) {
		MsgBoxError(NULL, _T("Dialog Object Cast Failed"),
			_T("Failed to get dialog object from event handler argument."));
		return;
	}

	// Convert the hostname to UTF-16.
	szHostname = utf16_mbstowcs(peer->name);
	if (szHostname == NULL) {
		MsgBoxLastError(NULL);
		return;
	}

	// Setup a peer object and append it to the list.
	glPeer = new GroundLift::Peer(_T("?"), szHostname,
		reinterpret_cast<const struct sockaddr *>(& peer->sock->addr_in),
		peer->sock->addr_in_size);
	pThis->AppendPeerToList(glPeer);

	// Free up any temporary resources.
	free(szHostname);
}
