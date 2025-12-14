/*
 * Copyright (c) 2024. Bert Laverman
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


#include "framework.h"
#include "WindowsMessaging.h"

#include <SimConnect.h>
#include <stdio.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    DlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     [[maybe_unused]] _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSMESSAGING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    [[maybe_unused]]
    LRESULT dlgTest = DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DlgProc);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSMESSAGING));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = DlgProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSMESSAGING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSMESSAGING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

static HANDLE hSimConnect = nullptr;
#define WM_USER_SIMCONNECT (WM_USER + 1)

//
// Build a string version of the given major and minor numbers.
// 
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

//
// Set the given dialog element to the given text
// 
static void SetDialogText(HWND hWnd, int id, const wchar_t* text) {
    HWND control = GetDlgItem(hWnd, id);
	SetWindowText(control, text); //NOLINT(MSVC)
    EnableWindow(control, TRUE);
}

//
// Convert the regular string to wide and set the given dialog element to it
//
static void SetDialogText(HWND hWnd, int id, const char* text) {
	int size = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    wchar_t* buf = nullptr;
    try {
		buf = new wchar_t[size];
        MultiByteToWideChar(CP_ACP, 0, text, -1, buf, 256);
        SetDialogText(hWnd, id, buf);
        delete[] buf;
    }
    catch (...) {
        delete[] buf;
        throw;
	}
}

//
// Set the given dialog item to "Unknown" and disable it.
//
static void SetDialogUnknown(HWND hWnd, int id) {
    HWND control = GetDlgItem(hWnd, id);
    SetWindowText(control, L"Unknown");
    EnableWindow(control, FALSE);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND connectBtn = GetDlgItem(hWnd, IDC_BTN_CONNECT);
    HWND disconnectBtn = GetDlgItem(hWnd, IDC_BTN_DISCONNECT);

    SIMCONNECT_RECV* msg = nullptr;
    DWORD msgSize = 0;

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDC_BTN_CONNECT:
                if (SUCCEEDED(SimConnect_Open(&hSimConnect, "WindowsMessaging", hWnd, WM_USER_SIMCONNECT, NULL, 0))) {
					EnableWindow(disconnectBtn, TRUE);
					EnableWindow(connectBtn, FALSE);
                    SetDialogText(hWnd, IDC_VAL_CON_STATUS, L"Connected");
                }
                else {
                    SetDialogText(hWnd, IDC_VAL_CON_STATUS, L"Failed to connect");
                }
                break;

			case IDC_BTN_DISCONNECT:
				SimConnect_Close(hSimConnect);
                hSimConnect = nullptr;

				EnableWindow(connectBtn, TRUE);
				EnableWindow(disconnectBtn, FALSE);
                SetDialogText(hWnd, IDC_VAL_CON_STATUS, L"Disconnected");
                
                SetDialogUnknown(hWnd, IDC_VAL_SIM_NAME);
                SetDialogUnknown(hWnd, IDC_VAL_SIM_TYPE);
                SetDialogUnknown(hWnd, IDC_VAL_SIM_VERSION);
                SetDialogUnknown(hWnd, IDC_VAL_SIM_BUILD);
                SetDialogUnknown(hWnd, IDC_VAL_SCN_VERSION);
                SetDialogUnknown(hWnd, IDC_VAL_SCN_BUILD);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_USER_SIMCONNECT:
        while (SUCCEEDED(SimConnect_GetNextDispatch(hSimConnect, &msg, &msgSize))) {
            switch (msg->dwID) {
            case SIMCONNECT_RECV_ID_OPEN:
            {
                SIMCONNECT_RECV_OPEN* open = (SIMCONNECT_RECV_OPEN*)msg;

                SetDialogText(hWnd, IDC_VAL_CON_STATUS, L"Connected, open received");

                SetDialogText(hWnd, IDC_VAL_SIM_NAME, open->szApplicationName);

                if (strncmp(open->szApplicationName, "KittyHawk", 10) == 0) {
                    SetDialogText(hWnd, IDC_VAL_SIM_TYPE, L"Microsoft Flight Simulator 2020");
                }
                else if (strncmp(open->szApplicationName, "SunRise", 8) == 0) {
                    SetDialogText(hWnd, IDC_VAL_SIM_TYPE, L"Microsoft Flight Simulator 2024");
                }
                else if (strncmp(open->szApplicationName, "Lockheed Martin", 16) == 0) {
                    SetDialogText(hWnd, IDC_VAL_SIM_TYPE, L"Lockheed Martin Prepar3D");
                }
                else {
                    SetDialogText(hWnd, IDC_VAL_SIM_TYPE, L"Unknown FlightSimulator");
                }

                char buf[256];

                SetDialogText(hWnd, IDC_VAL_SIM_VERSION, BuildVersionString(buf, sizeof(buf), open->dwApplicationVersionMajor, open->dwApplicationVersionMinor));
                SetDialogText(hWnd, IDC_VAL_SIM_BUILD, BuildVersionString(buf, sizeof(buf), open->dwApplicationBuildMajor, open->dwApplicationBuildMinor));

				SetDialogText(hWnd, IDC_VAL_SCN_VERSION, BuildVersionString(buf, sizeof(buf), open->dwSimConnectVersionMajor, open->dwSimConnectVersionMinor));
                SetDialogText(hWnd, IDC_VAL_SCN_BUILD, BuildVersionString(buf, sizeof(buf), open->dwSimConnectBuildMajor, open->dwSimConnectBuildMinor));

                break;
            }
            case SIMCONNECT_RECV_ID_QUIT:
				SetDialogText(hWnd, IDC_VAL_CON_STATUS, L"Disconnected, quit received");
                hSimConnect = nullptr;
                SetDialogUnknown(hWnd, IDC_VAL_SIM_NAME);
				SetDialogUnknown(hWnd, IDC_VAL_SIM_TYPE);
				SetDialogUnknown(hWnd, IDC_VAL_SIM_VERSION);
				SetDialogUnknown(hWnd, IDC_VAL_SIM_BUILD);
				SetDialogUnknown(hWnd, IDC_VAL_SCN_VERSION);
				SetDialogUnknown(hWnd, IDC_VAL_SCN_BUILD);

                EnableWindow(connectBtn, TRUE);
                EnableWindow(disconnectBtn, FALSE);
                break;

            default:
                // Unknown message
                break;
            }
        }

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
