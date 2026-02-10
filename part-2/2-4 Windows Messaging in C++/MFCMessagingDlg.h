/*
 * Copyright (c) 2026. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// MFCMessagingDlg.h : header file
//

#pragma once

#include <simconnect/windows_messaging_connection.hpp>
#include<simconnect/simple_handler.hpp>


// CMFCMessagingDlg dialog
class CMFCMessagingDlg : public CDialogEx
{
	CButton m_btnConnect;
	CButton m_btnDisconnect;

	CStatic m_conStatus;
	CStatic m_simName;
	CStatic m_simVersion;
	CStatic m_simBuild;
	CStatic m_simType;
	CStatic m_simConnectVersion;
	CStatic m_simConnectBuild;

	SimConnect::WindowsMessagingConnection<> m_connection;
	SimConnect::SimpleHandler<SimConnect::WindowsMessagingConnection<>> m_handler;

	constexpr static auto WM_SIMCONNECT = WM_USER + 1;

	void SetUnknown(CStatic& control) {
		control.SetWindowText(L"Unknown");
		control.EnableWindow(FALSE);
	}
	void SetText(CStatic& control, PCWSTR text) {
		control.SetWindowText(text);
		control.EnableWindow(TRUE);
	}

	void onOpen(const SIMCONNECT_RECV_OPEN& msg);
	void onQuit();

	// Construction
public:
	CMFCMessagingDlg();
	CMFCMessagingDlg(CWnd* pParent);

	virtual ~CMFCMessagingDlg() {}
	CMFCMessagingDlg(const CMFCMessagingDlg&) = delete;
	CMFCMessagingDlg(CMFCMessagingDlg&&) = delete;
	CMFCMessagingDlg& operator=(const CMFCMessagingDlg&) = delete;
	CMFCMessagingDlg& operator=(CMFCMessagingDlg&&) = delete;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAIN };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnSimConnectMessage(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
};
