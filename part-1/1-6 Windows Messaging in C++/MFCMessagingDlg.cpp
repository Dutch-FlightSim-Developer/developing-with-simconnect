
// MFCMessagingDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFCMessaging.h"
#include "MFCMessagingDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMFCMessagingDlg dialog


CMFCMessagingDlg::CMFCMessagingDlg()
	: CDialogEx(IDD_DIALOG_MAIN, nullptr)
	, m_connection(m_hWnd, WM_SIMCONNECT)
	, m_handler(m_connection)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_handler.registerHandlerProc(
		SIMCONNECT_RECV_ID_OPEN,
		[this](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD len) { this->onOpen(*reinterpret_cast<const SIMCONNECT_RECV_OPEN*>(msg)); });
	m_handler.registerHandlerProc(
		SIMCONNECT_RECV_ID_QUIT,
		[this]([[maybe_unused]] const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD len) { this->onQuit(); });
}

CMFCMessagingDlg::CMFCMessagingDlg(CWnd* pParent)
	: CDialogEx(IDD_DIALOG_MAIN, pParent)
	, m_connection(m_hWnd, WM_SIMCONNECT)
	, m_handler(m_connection)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_handler.registerHandlerProc(
		SIMCONNECT_RECV_ID_OPEN,
		[this](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD len) { this->onOpen(*reinterpret_cast<const SIMCONNECT_RECV_OPEN*>(msg)); });
	m_handler.registerHandlerProc(
		SIMCONNECT_RECV_ID_QUIT,
		[this]([[maybe_unused]] const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD len) { this->onQuit(); });
}


void CMFCMessagingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CONNECT, m_btnConnect);
	DDX_Control(pDX, IDC_BTN_DISCONNECT, m_btnDisconnect);
	DDX_Control(pDX, IDC_VAL_CON_STATUS, m_conStatus);
	DDX_Control(pDX, IDC_VAL_SIM_NAME, m_simName);
	DDX_Control(pDX, IDC_VAL_SIM_VERSION, m_simVersion);
	DDX_Control(pDX, IDC_VAL_SIM_BUILD, m_simBuild);
	DDX_Control(pDX, IDC_VAL_SIM_TYPE, m_simType);
	DDX_Control(pDX, IDC_VAL_SCN_VERSION, m_simConnectVersion);
	DDX_Control(pDX, IDC_VAL_SCN_BUILD, m_simConnectBuild);
}

BEGIN_MESSAGE_MAP(CMFCMessagingDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CMFCMessagingDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CMFCMessagingDlg::OnBnClickedBtnDisconnect)
	ON_MESSAGE(WM_SIMCONNECT, &CMFCMessagingDlg::OnSimConnectMessage)
END_MESSAGE_MAP()


// CMFCMessagingDlg message handlers

BOOL CMFCMessagingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCMessagingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCMessagingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/**
 * Somebody clicked "Connect". Note that the button is only enabled while disconnected.
 */
void CMFCMessagingDlg::OnBnClickedBtnConnect()
{
	if (m_connection.open(m_hWnd, WM_SIMCONNECT)) {
		m_conStatus.SetWindowText(L"Connected");
		m_btnConnect.EnableWindow(false);
		m_btnDisconnect.EnableWindow(true);
	}
	else {
		m_conStatus.SetWindowText(L"Connection Failed");
	}
}


/**
 * Somebody clicked "Disconnect". Note that the button is only enabled while connected.
 */
void CMFCMessagingDlg::OnBnClickedBtnDisconnect()
{
	m_connection.close();
	m_conStatus.SetWindowText(L"Disconnected");

	m_btnConnect.EnableWindow(true);
	m_btnDisconnect.EnableWindow(false);

	SetUnknown(m_simName);
	SetUnknown(m_simVersion);
	SetUnknown(m_simBuild);
	SetUnknown(m_simType);
	SetUnknown(m_simConnectVersion);
	SetUnknown(m_simConnectBuild);
}


/**
 * WM_SIMCONNECT message handler.
 */
LRESULT CMFCMessagingDlg::OnSimConnectMessage(WPARAM wParam, LPARAM lParam) {
	m_handler.dispatch();
	return 0;
}


/**
 * Builds a version string from major and minor version numbers.
 *
 * @param buf the buffer to print to.
 * @param bufSize the size of the buffer.
 * @param major the major version number.
 * @param minor the minor version number.
 */
static const char* BuildVersionString(char* buf, unsigned bufSize, int major, int minor) {
	if (major == 0) {
		strncpy_s(buf, bufSize, "Unknown", bufSize - 1);
	}
	else if (minor == 0) {
		snprintf(buf, bufSize, "%d", major);
	}
	else {
		snprintf(buf, bufSize, "%d.%d", major, minor);
	}
	return buf;
}


/**
 * Handles the SIMCONNECT_RECV_OPEN message.
 */
void CMFCMessagingDlg::onOpen(const SIMCONNECT_RECV_OPEN& msg) {
	m_conStatus.SetWindowTextW(L"Connected, open received.");

	CString simName, simVersion, simBuild, simType, scnVersion, scnBuild;
	char buf[256];

	simName = msg.szApplicationName;
	SetText(m_simName, simName);

	simVersion = BuildVersionString(buf, sizeof(buf), msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor);
	SetText(m_simVersion, simVersion);

	simBuild = BuildVersionString(buf, sizeof(buf), msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor);
	SetText(m_simBuild, simBuild);

	if (strncmp(msg.szApplicationName, "KittyHawk", 10) == 0) {
		simType = L"Microsoft Flight Simulator 2020";
	}
	else if (strncmp(msg.szApplicationName, "SunRise", 8) == 0) {
		simType = L"Microsoft Flight Simulator 2024";
	}
	else if (strncmp(msg.szApplicationName, "Lockheed Martin", 16) == 0) {
		simType = L"Lockheed Martin Prepar3D";
	}
	else {
		simType = L"Unknown FlightSimulator";
	}
	SetText(m_simType, simType);

	scnVersion = BuildVersionString(buf, sizeof(buf), msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor);
	SetText(m_simConnectVersion, scnVersion);

	scnBuild = BuildVersionString(buf, sizeof(buf), msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor);
	SetText(m_simConnectBuild, scnBuild);
}


/**
 * Handles the SIMCONNECT_RECV_QUIT message.
 */
void CMFCMessagingDlg::onQuit() {
	m_conStatus.SetWindowTextW(L"Disconnected, quit received.");

	m_btnConnect.EnableWindow(true);
	m_btnDisconnect.EnableWindow(false);
	m_connection.close();

	SetUnknown(m_simName);
	SetUnknown(m_simVersion);
	SetUnknown(m_simBuild);
	SetUnknown(m_simType);
	SetUnknown(m_simConnectVersion);
	SetUnknown(m_simConnectBuild);
}