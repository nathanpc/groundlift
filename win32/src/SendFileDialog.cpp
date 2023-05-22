/**
 * SendFileDialog.cpp
 * A dialog to send files to a specific client.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "SendFileDialog.h"

#include "CommonIncludes.h"

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
}

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

	// TODO: Ensure we get the callback for the discovery service.

	// TODO: Send discovery broadcast.

	// TODO: Populate the list.

	AppendPeerToList(
		new GroundLift::Peer(_T("W"), _T("You"), _T("127.0.0.1"), 1650));
	AppendPeerToList(
		new GroundLift::Peer(_T("L"), _T("Some Device"), _T("192.168.1.2"), 1650));
	AppendPeerToList(
		new GroundLift::Peer(_T("W"), _T("Win98"), _T("192.168.1.13"), 1650));
	AppendPeerToList(
		new GroundLift::Peer(_T("A"), _T("iPhone"), _T("192.168.1.66"), 1650));
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
	lvi.iSubItem = this->kDeviceTypeColIndex;
	lvi.pszText = const_cast<LPTSTR>(szDeviceType);
	ListView_InsertItem(this->hwndPeerList, &lvi);

	// Setup and add the Hostname cell.
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = this->kHostnameColIndex;
	lvi.pszText = const_cast<LPTSTR>(szHostname);
	ListView_SetItem(this->hwndPeerList, &lvi);

	// Setup and add the IP Address cell.
	lvi.iSubItem = this->kIPAddressColIndex;
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
				case IDOK:
					//MsgBox(hDlg, MB_OK, L"Send File", L"Send the file!");
					break;
				case IDCANCEL:
					//MsgBox(hDlg, MB_OK, L"Send File", L"Cancel everything!");
					break;
				case IDC_BUTTON_BROWSE:
					break;
			}
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
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
	lvc.iSubItem = this->kDeviceTypeColIndex;
	ListView_InsertColumn(this->hwndPeerList, lvc.iSubItem, &lvc);

	// Setup the hostname column.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 140;
	lvc.pszText = const_cast<LPTSTR>(_T("Hostname"));
	lvc.iSubItem = this->kHostnameColIndex;
	ListView_InsertColumn(this->hwndPeerList, lvc.iSubItem, &lvc);

	// Setup the IP address column.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 170;
	lvc.pszText = const_cast<LPTSTR>(_T("IP Address"));
	lvc.iSubItem = this->kIPAddressColIndex;
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
