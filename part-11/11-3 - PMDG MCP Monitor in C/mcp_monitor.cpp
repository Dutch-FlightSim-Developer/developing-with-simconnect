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


// PMDG 737 NG3 - MCP Monitor
//
// Connects to SimConnect and subscribes to the PMDG_NG3_Data client data area
// using SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET with the CHANGED flag, so we only
// receive a delivery when PMDG actually writes new values.
//
// On each delivery we compare the five MCP display fields against our last
// known state and print any that have changed:
//
//   Heading     (MCP_Heading)
//   Altitude    (MCP_Altitude, in feet)
//   Speed       (MCP_IASMach / MCP_IASBlank — IAS knots or Mach number)
//   Vert Speed  (MCP_VertSpeed / MCP_VertSpeedBlank, in fpm)
//
// Prerequisites
//   - MSFS running with the PMDG 737 loaded.
//   - [SDK] EnableDataBroadcast=1 in 737NG3_Options.ini
//     (typically found in the PMDG work folder inside your Community folder).

#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#pragma warning(push, 3)
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <format>
#include <iostream>


static constexpr const char* appName{ "PMDG NG3 MCP Monitor" };

static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };

static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_PMDG_DATA{ 1 };


// Last seen MCP state — sentinel values mean "not yet received".
struct McpState {
    bool           powered{ false };
    unsigned short heading{ 0xFFFFu };
    unsigned short altitude{ 0xFFFFu };
    float          iasMach{ -1.0f };
    bool           iasBlank{ true };
    short          vertSpeed{ 0 };
    bool           vsBlank{ true };
};

static McpState g_mcp;


static void processMcpData(const PMDG_NG3_Data* pS)
{
    if (!pS->MCP_indication_powered) {
        if (g_mcp.powered) {
            g_mcp.powered = false;
            std::cout << "[MCP] Power OFF\n";
        }
        return;
    }

    if (!g_mcp.powered) {
        g_mcp.powered = true;
        std::cout << "[MCP] Power ON\n";
    }

    if (pS->MCP_Heading != g_mcp.heading) {
        g_mcp.heading = pS->MCP_Heading;
        std::cout << std::format("[MCP] Heading:    {:03}\n", g_mcp.heading);
    }

    if (pS->MCP_Altitude != g_mcp.altitude) {
        g_mcp.altitude = pS->MCP_Altitude;
        std::cout << std::format("[MCP] Altitude:   {} ft\n", g_mcp.altitude);
    }

    if (pS->MCP_IASBlank != g_mcp.iasBlank || pS->MCP_IASMach != g_mcp.iasMach) {
        g_mcp.iasBlank = pS->MCP_IASBlank;
        g_mcp.iasMach  = pS->MCP_IASMach;
        if (g_mcp.iasBlank) {
            std::cout << "[MCP] Speed:      ---\n";
        } else if (g_mcp.iasMach < 10.0f) {
            std::cout << std::format("[MCP] Speed:      M{:.2f}\n", g_mcp.iasMach);
        } else {
            std::cout << std::format("[MCP] Speed:      {:.0f} kts\n", g_mcp.iasMach);
        }
    }

    if (pS->MCP_VertSpeedBlank != g_mcp.vsBlank || pS->MCP_VertSpeed != g_mcp.vertSpeed) {
        g_mcp.vsBlank   = pS->MCP_VertSpeedBlank;
        g_mcp.vertSpeed = pS->MCP_VertSpeed;
        if (g_mcp.vsBlank) {
            std::cout << "[MCP] Vert Speed: ---\n";
        } else {
            std::cout << std::format("[MCP] Vert Speed: {:+} fpm\n", static_cast<int>(g_mcp.vertSpeed));
        }
    }
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


static bool subscribeToMcpData()
{
    HRESULT hr;

    hr = SimConnect_MapClientDataNameToID(hSimConnect, PMDG_NG3_DATA_NAME, PMDG_NG3_DATA_ID);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map PMDG data name: 0x{:08X}]\n", hr);
        return false;
    }

    hr = SimConnect_AddToClientDataDefinition(hSimConnect, PMDG_NG3_DATA_DEFINITION,
        0, sizeof(PMDG_NG3_Data), 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add PMDG data definition: 0x{:08X}]\n", hr);
        return false;
    }

    // PERIOD_ON_SET + FLAG_CHANGED: receive a delivery only when PMDG writes
    // new values into the area, and only when at least one byte has changed.
    hr = SimConnect_RequestClientData(hSimConnect,
        PMDG_NG3_DATA_ID, REQ_PMDG_DATA, PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
        0, 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to subscribe to PMDG data: 0x{:08X}]\n", hr);
        return false;
    }

    return true;
}


static void runMessageLoop()
{
    std::cerr << "[Monitoring MCP displays. Press Ctrl+C to quit.]\n\n";

    while (true) {
        const DWORD waitResult = WaitForSingleObject(hEvent, 100);
        if (waitResult == WAIT_FAILED) {
            std::cerr << std::format("[WaitForSingleObject failed: 0x{:08X}]\n", GetLastError());
            return;
        }

        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };

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
                const auto* pClientData = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(pData);
                if (pClientData->dwRequestID == REQ_PMDG_DATA) {
                    processMcpData(reinterpret_cast<const PMDG_NG3_Data*>(&pClientData->dwData));
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


int main()
{
    if (!connect()) {
        return 1;
    }
    std::cerr << "[Connected to Flight Simulator.]\n";

    if (!subscribeToMcpData()) {
        disconnect();
        return 1;
    }

    runMessageLoop();
    disconnect();
    return 0;
}
