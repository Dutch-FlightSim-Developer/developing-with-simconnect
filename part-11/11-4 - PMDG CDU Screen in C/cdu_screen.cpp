/*
 * Copyright (c) 2025. Bert Laverman
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


// PMDG 737 NG3 - CDU Screen Monitor
//
// Renders one CDU screen (left or right) in the console using ANSI escape
// sequences for cursor positioning and color, updated on every ON_SET delivery.
//
// Usage:
//   cdu_screen [0|l|left]      left CDU  (default)
//   cdu_screen  1|r|right      right CDU
//
// Prerequisites:
//   - MSFS running with the PMDG 737 loaded.
//   - In 737NG3_Options.ini, [SDK] section:
//       EnableDataBroadcast=1
//       EnableCDUBroadcast.0=1      (for left CDU)
//       EnableCDUBroadcast.1=1      (for right CDU)
//
// Color mapping (ANSI nearest match):
//   WHITE   -> bright white   CYAN -> bright cyan   GREEN -> bright green
//   MAGENTA -> bright magenta AMBER -> dark yellow   RED   -> bright red
//
// Flag rendering:
//   SMALL_FONT -> lowercase alpha characters (label/header rows have no font-size in a console)
//   REVERSE    -> ANSI reverse video
//   UNUSED     -> ANSI dim


#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#pragma warning(push, 3)
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <cctype>
#include <format>
#include <iostream>
#include <string>
#include <string_view>


static constexpr const char* appName{ "PMDG NG3 CDU Screen" };

static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };
static int    g_cduIndex{ 0 };   // 0 = left, 1 = right

static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_CDU{ 1 };

// Console layout: title on row 1, CDU starts at row 2, col 2 (1-based).
static constexpr int CONSOLE_ROW_OFFSET{ 2 };
static constexpr int CONSOLE_COL_OFFSET{ 2 };

// ANSI escape sequences
static constexpr std::string_view ANSI_RESET       { "\033[0m"    };
static constexpr std::string_view ANSI_BG_BLACK    { "\033[40m"   };
static constexpr std::string_view ANSI_DIM         { "\033[2m"    };
static constexpr std::string_view ANSI_REVERSE     { "\033[7m"    };
static constexpr std::string_view ANSI_CURSOR_HIDE { "\033[?25l"  };
static constexpr std::string_view ANSI_CURSOR_SHOW { "\033[?25h"  };
static constexpr std::string_view ANSI_CLEAR       { "\033[2J"    };

// Foreground color sequences indexed by PMDG_NG3_CDU_COLOR_* values
static constexpr const char* CDU_FG_COLORS[] = {
    "\033[97m",   // 0 WHITE   — bright white
    "\033[96m",   // 1 CYAN    — bright cyan
    "\033[92m",   // 2 GREEN   — bright green
    "\033[95m",   // 3 MAGENTA — bright magenta
    "\033[33m",   // 4 AMBER   — dark yellow (closest ANSI match)
    "\033[91m",   // 5 RED     — bright red
};
static constexpr int NUM_CDU_COLORS{ static_cast<int>(std::size(CDU_FG_COLORS)) };


static void enableAnsiVT()
{
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode{ 0 };
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

static void moveTo(int row, int col)
{
    std::cout << std::format("\033[{};{}H", row, col);
}

static void printTitle()
{
    moveTo(1, 1);
    std::cout << ANSI_RESET
              << std::format("  PMDG 737  CDU {}  (Ctrl+C to quit)",
                             g_cduIndex == 0 ? "LEFT " : "RIGHT");
}

static void renderScreen(const PMDG_NG3_CDU_Screen* pScreen)
{
    std::cout << ANSI_CURSOR_HIDE;

    if (!pScreen->Powered) {
        // Clear the CDU area and show an off indicator.
        for (int row = 0; row < CDU_ROWS; ++row) {
            moveTo(CONSOLE_ROW_OFFSET + row, CONSOLE_COL_OFFSET);
            std::cout << ANSI_RESET << ANSI_BG_BLACK
                      << std::string(CDU_COLUMNS, ' ') << ANSI_RESET;
        }
        moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS / 2, CONSOLE_COL_OFFSET + 7);
        std::cout << "\033[90m[ CDU OFF ]" << ANSI_RESET;
    }
    else {
        // Cells[col][row] — iterate row-major for output.
        for (int row = 0; row < CDU_ROWS; ++row) {
            moveTo(CONSOLE_ROW_OFFSET + row, CONSOLE_COL_OFFSET);
            std::cout << ANSI_BG_BLACK;

            for (int col = 0; col < CDU_COLUMNS; ++col) {
                const PMDG_NG3_CDU_Cell& cell = pScreen->Cells[col][row];

                // Attributes: reset first, then apply flags and color.
                std::cout << ANSI_RESET << ANSI_BG_BLACK;

                if (cell.Flags & PMDG_NG3_CDU_FLAG_REVERSE) {
                    std::cout << ANSI_REVERSE;
                }

                const int colorIdx = (cell.Color < NUM_CDU_COLORS) ? cell.Color : 0;
                std::cout << CDU_FG_COLORS[colorIdx];

                if (cell.Flags & PMDG_NG3_CDU_FLAG_UNUSED) {
                    std::cout << ANSI_DIM;
                }

                // Printable ASCII range only; everything else is a space.
                char sym = (cell.Symbol >= 0x20 && cell.Symbol < 0x7F)
                    ? static_cast<char>(cell.Symbol) : ' ';

                // SMALL_FONT marks label/header rows — render as lowercase alpha
                // as a visual hint, since we can't change the font size.
                if ((cell.Flags & PMDG_NG3_CDU_FLAG_SMALL_FONT) && std::isalpha(static_cast<unsigned char>(sym))) {
                    sym = static_cast<char>(std::tolower(static_cast<unsigned char>(sym)));
                }

                std::cout << sym;
            }
            std::cout << ANSI_RESET;
        }
    }

    // Park the cursor below the CDU area and flush.
    moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS + 1, 1);
    std::cout << ANSI_CURSOR_SHOW;
    std::cout.flush();
}


static bool connect()
{
    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hEvent == nullptr) {
        std::cerr << std::format("[Failed to create Windows event: 0x{:08X}]\n", GetLastError());
        return false;
    }

    const HRESULT hr = SimConnect_Open(&hSimConnect, appName, nullptr, 0, hEvent, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to connect to SimConnect: 0x{:08X}]\n", hr);
        return false;
    }

    return true;
}

static void disconnect()
{
    if (hSimConnect != nullptr) {
        SimConnect_Close(hSimConnect);
        hSimConnect = nullptr;
    }
    if (hEvent != nullptr) {
        CloseHandle(hEvent);
        hEvent = nullptr;
    }
}


static bool subscribeToCdu()
{
    const char* const cduName       = (g_cduIndex == 0) ? PMDG_NG3_CDU_0_NAME       : PMDG_NG3_CDU_1_NAME;
    const DWORD       cduId         = (g_cduIndex == 0) ? PMDG_NG3_CDU_0_ID         : PMDG_NG3_CDU_1_ID;
    const DWORD       cduDefinition = (g_cduIndex == 0) ? PMDG_NG3_CDU_0_DEFINITION : PMDG_NG3_CDU_1_DEFINITION;

    HRESULT hr;

    hr = SimConnect_MapClientDataNameToID(hSimConnect, cduName, cduId);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map CDU name to ID: 0x{:08X}]\n", hr);
        return false;
    }

    hr = SimConnect_AddToClientDataDefinition(hSimConnect, cduDefinition,
        0, sizeof(PMDG_NG3_CDU_Screen), 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add CDU data definition: 0x{:08X}]\n", hr);
        return false;
    }

    // PERIOD_ON_SET + FLAG_CHANGED: only wake up when PMDG writes new screen data.
    hr = SimConnect_RequestClientData(hSimConnect,
        cduId, REQ_CDU, cduDefinition,
        SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
        0, 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to subscribe to CDU data: 0x{:08X}]\n", hr);
        return false;
    }

    return true;
}


static void runMessageLoop()
{
    while (true) {
        const DWORD waitResult = WaitForSingleObject(hEvent, 100);
        if (waitResult == WAIT_FAILED) {
            std::cerr << std::format("[WaitForSingleObject failed: 0x{:08X}]\n", GetLastError());
            return;
        }

        SIMCONNECT_RECV* pData{ nullptr };
        DWORD            cbData{ 0 };

        while (SUCCEEDED(SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            switch (pData->dwID) {

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const auto* pOpen = reinterpret_cast<const SIMCONNECT_RECV_OPEN*>(pData);
                std::cerr << std::format("[Connected to '{}' version {}.{}]\n",
                    pOpen->szApplicationName,
                    pOpen->dwApplicationVersionMajor, pOpen->dwApplicationVersionMinor);
                break;
            }

            case SIMCONNECT_RECV_ID_CLIENT_DATA:
            {
                const auto* pCd = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(pData);
                if (pCd->dwRequestID == REQ_CDU) {
                    renderScreen(reinterpret_cast<const PMDG_NG3_CDU_Screen*>(&pCd->dwData));
                }
                break;
            }

            case SIMCONNECT_RECV_ID_EXCEPTION:
            {
                const auto* pEx = reinterpret_cast<const SIMCONNECT_RECV_EXCEPTION*>(pData);
                std::cerr << std::format("[SimConnect exception: {}]\n", pEx->dwException);
                break;
            }

            case SIMCONNECT_RECV_ID_QUIT:
                std::cerr << "[Simulator is shutting down.]\n";
                return;

            default:
                break;
            }
        }
    }
}


int main(int argc, char* argv[])
{
    if (argc >= 2) {
        const std::string arg{ argv[1] };
        if (arg == "1" || arg == "r" || arg == "right") {
            g_cduIndex = 1;
        } else if (arg == "0" || arg == "l" || arg == "left") {
            g_cduIndex = 0;
        } else {
            std::cerr << std::format("Usage: {} [0|l|left | 1|r|right]\n", argv[0]);
            return 1;
        }
    }

    enableAnsiVT();
    std::cout << ANSI_CLEAR << ANSI_BG_BLACK;
    printTitle();

    if (!connect()) {
        return 1;
    }

    if (!subscribeToCdu()) {
        disconnect();
        return 1;
    }

    runMessageLoop();

    // Restore terminal state on exit.
    moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS + 2, 1);
    std::cout << ANSI_RESET << ANSI_CURSOR_SHOW;
    std::cout.flush();

    disconnect();
    return 0;
}
