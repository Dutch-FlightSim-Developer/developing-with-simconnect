/*
 * Diagnostic: dump all ELEC_* fields from PMDG_NG3_Data on every change.
 * Flip switches in the cockpit and watch the values update.
 */

#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#pragma warning(push, 3)
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <format>
#include <iostream>

static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };

static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_INITIAL{ 1 };
static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_UPDATES{ 2 };

static constexpr std::string_view ANSI_CLEAR{ "\033[2J\033[H" };

static void dump(const PMDG_NG3_Data& d)
{
    std::cout << ANSI_CLEAR
              << "PMDG 737 — ELEC fields (Ctrl+C to quit)\n"
              << std::string(44, '-') << "\n\n"
              << std::format("  ELEC_annunBAT_DISCHARGE          {}\n",  d.ELEC_annunBAT_DISCHARGE)
              << std::format("  ELEC_annunTR_UNIT                {}\n",  d.ELEC_annunTR_UNIT)
              << std::format("  ELEC_annunELEC                   {}\n",  d.ELEC_annunELEC)
              << std::format("  ELEC_BatSelector                 {}  (0=OFF 1=BAT 2=ON)\n",  d.ELEC_BatSelector)
              << std::format("  ELEC_DCMeterSelector             {}\n",  d.ELEC_DCMeterSelector)
              << std::format("  ELEC_ACMeterSelector             {}\n",  d.ELEC_ACMeterSelector)
              << std::format("  ELEC_CabUtilSw                   {}\n",  d.ELEC_CabUtilSw)
              << std::format("  ELEC_IFEPassSeatSw               {}\n",  d.ELEC_IFEPassSeatSw)
              << std::format("  ELEC_annunDRIVE[0/1]             {} {}\n", d.ELEC_annunDRIVE[0], d.ELEC_annunDRIVE[1])
              << std::format("  ELEC_annunSTANDBY_POWER_OFF      {}\n",  d.ELEC_annunSTANDBY_POWER_OFF)
              << std::format("  ELEC_IDGDisconnectSw[0/1]        {} {}\n", d.ELEC_IDGDisconnectSw[0], d.ELEC_IDGDisconnectSw[1])
              << std::format("  ELEC_StandbyPowerSelector        {}  (0=BAT 1=OFF 2=AUTO)\n", d.ELEC_StandbyPowerSelector)
              << std::format("  ELEC_annunGRD_POWER_AVAILABLE    {}\n",  d.ELEC_annunGRD_POWER_AVAILABLE)
              << std::format("  ELEC_GrdPwrSw                    {}\n",  d.ELEC_GrdPwrSw)
              << std::format("  ELEC_BusTransSw_AUTO             {}\n",  d.ELEC_BusTransSw_AUTO)
              << std::format("  ELEC_GenSw[0/1]                  {} {}\n", d.ELEC_GenSw[0], d.ELEC_GenSw[1])
              << std::format("  ELEC_APUGenSw[0/1]               {} {}\n", d.ELEC_APUGenSw[0], d.ELEC_APUGenSw[1])
              << std::format("  ELEC_annunTRANSFER_BUS_OFF[0/1]  {} {}\n", d.ELEC_annunTRANSFER_BUS_OFF[0], d.ELEC_annunTRANSFER_BUS_OFF[1])
              << std::format("  ELEC_annunSOURCE_OFF[0/1]        {} {}\n", d.ELEC_annunSOURCE_OFF[0], d.ELEC_annunSOURCE_OFF[1])
              << std::format("  ELEC_annunGEN_BUS_OFF[0/1]       {} {}\n", d.ELEC_annunGEN_BUS_OFF[0], d.ELEC_annunGEN_BUS_OFF[1])
              << std::format("  ELEC_annunAPU_GEN_OFF_BUS        {}\n",  d.ELEC_annunAPU_GEN_OFF_BUS)
              << std::format("  ELEC_MeterDisplayTop             \"{}\"\n", d.ELEC_MeterDisplayTop)
              << std::format("  ELEC_MeterDisplayBottom          \"{}\"\n", d.ELEC_MeterDisplayBottom)
              << "\n";

    std::cout.flush();
}

int main()
{
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode{ 0 };
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    const HRESULT hr = SimConnect_Open(&hSimConnect, "PMDG ELEC Dump", nullptr, 0, hEvent, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[SimConnect_Open failed: 0x{:08X}]\n", hr);
        return 1;
    }

    SimConnect_MapClientDataNameToID(hSimConnect, PMDG_NG3_DATA_NAME, PMDG_NG3_DATA_ID);
    SimConnect_AddToClientDataDefinition(hSimConnect, PMDG_NG3_DATA_DEFINITION,
        0, sizeof(PMDG_NG3_Data), 0, 0);

    // Initial snapshot.
    SimConnect_RequestClientData(hSimConnect,
        PMDG_NG3_DATA_ID, REQ_INITIAL, PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_ONCE, 0, 0, 0, 0);

    // Live updates.
    SimConnect_RequestClientData(hSimConnect,
        PMDG_NG3_DATA_ID, REQ_UPDATES, PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
        0, 0, 0);

    while (true) {
        WaitForSingleObject(hEvent, 100);

        SIMCONNECT_RECV* pData{ nullptr };
        DWORD            cbData{ 0 };

        while (SUCCEEDED(SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            if (pData->dwID == SIMCONNECT_RECV_ID_CLIENT_DATA) {
                const auto* pCd = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(pData);
                if (pCd->dwRequestID == REQ_INITIAL || pCd->dwRequestID == REQ_UPDATES) {
                    dump(*reinterpret_cast<const PMDG_NG3_Data*>(&pCd->dwData));
                }
            }
            else if (pData->dwID == SIMCONNECT_RECV_ID_QUIT) {
                SimConnect_Close(hSimConnect);
                return 0;
            }
        }
    }
}
