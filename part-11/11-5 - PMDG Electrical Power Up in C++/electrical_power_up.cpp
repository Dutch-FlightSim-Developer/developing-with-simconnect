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


// PMDG 737 NG3 - Electrical Power Up (Supplementary Procedures)
//
// Models the "Electrical Power Up" section of the 737 FCOM Supplementary
// Procedures as a C++ class.  Each step has three aspects:
//
//   isSatisfied(data)  — returns true when the item is already in the
//                        required position (the action has been taken).
//   execute(hSC)       — transmits the appropriate PMDG SDK event to set
//                        the correct position programmatically.
//   verifications      — post-condition checks (annunciators, gauges) that
//                        confirm the action was effective.
//
// The ElectricalPowerUpProcedure class owns the ordered list of steps and
// exposes a walkThrough() method that iterates the list and stops at the
// first step that has not yet been satisfied, returning its index.
//
// As an MVP the demo only monitors: it identifies the first pending step and
// reports it.  The execute() capability is wired up and ready for an active
// "auto-fix" extension.
//
// Procedure steps (FCOM Supplementary Procedures — Electrical Power Up):
//   1. BATTERY switch           = GUARDED (closed)
//   2. STANDBY POWER switch     = GUARDED (closed) / AUTO
//   3. ALTERNATE FLAPS master   = GUARDED (closed) / OFF
//   4. WIPERS                   = PARK (both)
//   5. ELECTRIC HYD PUMPS       = OFF (both)
//   6. LANDING GEAR lever       = DN
//         verify: gear green lights ON  (all three locked)
//         verify: gear red lights  OFF  (no gear in transit)
//
// Prerequisites:
//   - MSFS running with the PMDG 737 loaded.
//   - [SDK] EnableDataBroadcast=1 in 737NG3_Options.ini


#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#pragma warning(push, 3)
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <array>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>


static constexpr const char* appName{ "PMDG NG3 Electrical Power Up" };

static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };

static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_PMDG_DATA_INITIAL{ 1 }; // one-shot on connect
static constexpr SIMCONNECT_DATA_REQUEST_ID REQ_PMDG_DATA{ 2 };          // ongoing ON_SET updates

// Base SimConnect event ID for the procedure's PMDG events.
static constexpr SIMCONNECT_CLIENT_EVENT_ID PROC_EVENT_BASE{ 100 };

// ANSI escape sequences
static constexpr std::string_view ANSI_RESET  { "\033[0m"    };
static constexpr std::string_view ANSI_GREEN  { "\033[92m"   };
static constexpr std::string_view ANSI_RED    { "\033[91m"   };
static constexpr std::string_view ANSI_YELLOW { "\033[93m"   };
static constexpr std::string_view ANSI_CLEAR  { "\033[2J\033[H" };


// =============================================================================
// Procedure data structures
// =============================================================================

enum class PowerSource { Ground, APU };
enum class RunMode     { Once, Cycle };

struct VerificationCheck {
    std::string description;
    std::function<bool(const PMDG_NG3_Data&)> passes;
};

struct ProcedureStep {
    std::string item;           // e.g. "BATTERY switch"
    std::string requiredState;  // e.g. "GUARDED (closed)"
    std::function<bool(const PMDG_NG3_Data&)>        isSatisfied;
    std::function<std::string(const PMDG_NG3_Data&)> currentState;
    std::function<void(HANDLE)>                       execute;
    std::vector<VerificationCheck>                    verifications;
};


// =============================================================================
// ElectricalPowerUpProcedure
// =============================================================================

class ElectricalPowerUpProcedure {
public:
    explicit ElectricalPowerUpProcedure(SIMCONNECT_CLIENT_EVENT_ID firstEventId,
                                        PowerSource                source,
                                        bool                       skipFireTests = false)
        : firstEventId_{ firstEventId }, powerSource_{ source }, skipFireTests_{ skipFireTests }
    {
        buildSteps();
    }

    // Register all PMDG SDK events with SimConnect.
    // Call once, immediately after SimConnect_Open.
    void registerEvents(HANDLE hSC) const
    {
        for (DWORD i = 0; i < EventCount; ++i) {
            const std::string name = "#" + std::to_string(pmdgEventIds_[i]);
            SimConnect_MapClientEventToSimEvent(hSC, firstEventId_ + i, name.c_str());
        }
    }

    // Walk through all steps in order.
    // Returns the index of the first unsatisfied step, or stepCount() when all done.
    size_t walkThrough(const PMDG_NG3_Data& data) const
    {
        for (size_t i = 0; i < steps_.size(); ++i) {
            if (!steps_[i].isSatisfied(data)) {
                return i;
            }
        }
        return steps_.size();
    }

    // Fire the command event(s) for the step at the given index.
    void executeStep(size_t index, HANDLE hSC) const
    {
        if (index < steps_.size() && steps_[index].execute) {
            steps_[index].execute(hSC);
        }
    }

    // Run all verification checks for the step at the given index.
    // Returns descriptions of any checks that failed.
    std::vector<std::string> verifyStep(size_t index, const PMDG_NG3_Data& data) const
    {
        std::vector<std::string> failures;
        if (index < steps_.size()) {
            for (const auto& check : steps_[index].verifications) {
                if (!check.passes(data)) {
                    failures.push_back(check.description);
                }
            }
        }
        return failures;
    }

    size_t stepCount() const { return steps_.size(); }
    const ProcedureStep& step(size_t index) const { return steps_[index]; }

private:
    // Offsets into pmdgEventIds_[] / into the allocated SimConnect event ID range.
    enum EventOffset : DWORD {
        BatteryGuard = 0,
        StbyPwrSwitch,
        StbyPwrGuard,
        AltFlapsMasterSwitch,
        AltFlapsMasterGuard,
        WiperLeft,
        WiperRight,
        ElecHydPump1,
        ElecHydPump2,
        GearLever,
        GrdPwrSwitch,
        EventCount
    };

    // PMDG SDK event ID for each offset (THIRD_PARTY_EVENT_ID_MIN = 0x00011000 = 69632).
    static constexpr DWORD pmdgEventIds_[EventCount] = {
        EVT_OH_ELEC_BATTERY_GUARD,      // parameter: 1 = close, 0 = open
        EVT_OH_ELEC_STBY_PWR_SWITCH,    // parameter: 0 = BAT, 1 = OFF, 2 = AUTO
        EVT_OH_ELEC_STBY_PWR_GUARD,     // parameter: 1 = close, 0 = open
        EVT_OH_ALT_FLAPS_MASTER_SWITCH, // parameter: 0 = OFF (guarded), 1 = ARM
        EVT_OH_ALT_FLAPS_MASTER_GUARD,  // parameter: 1 = close, 0 = open
        EVT_OH_WIPER_LEFT_CONTROL,      // parameter: 0 = PARK, 1 = INT, 2 = LOW, 3 = HIGH
        EVT_OH_WIPER_RIGHT_CONTROL,     // parameter: 0 = PARK, 1 = INT, 2 = LOW, 3 = HIGH
        EVT_OH_HYD_ELEC1,               // parameter: 0 = OFF, 1 = ON
        EVT_OH_HYD_ELEC2,               // parameter: 0 = OFF, 1 = ON
        EVT_GEAR_LEVER,                 // parameter: 0 = UP, 1 = OFF, 2 = DOWN
        EVT_OH_ELEC_GRD_PWR_SWITCH,     // parameter: 1 = ON, 0 = OFF
    };

    SIMCONNECT_CLIENT_EVENT_ID firstEventId_;
    PowerSource                powerSource_;
    bool                       skipFireTests_;
    std::vector<ProcedureStep> steps_;

    SIMCONNECT_CLIENT_EVENT_ID eventId(EventOffset offset) const
    {
        return firstEventId_ + static_cast<SIMCONNECT_CLIENT_EVENT_ID>(offset);
    }

    void transmit(HANDLE hSC, EventOffset offset, DWORD parameter) const
    {
        SimConnect_TransmitClientEvent(hSC, 0, eventId(offset), parameter,
            SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }

    void buildSteps()
    {
        // Capture firstEventId_ by value so execute lambdas are self-contained.
        const SIMCONNECT_CLIENT_EVENT_ID base{ firstEventId_ };

        auto send = [base](HANDLE hSC, EventOffset offset, DWORD parameter) {
            SimConnect_TransmitClientEvent(hSC, 0, base + offset, parameter,
                SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        };

        // 1. BATTERY switch = GUARDED (closed)
        //    ELEC_BatSelector: 0=OFF (guard open), 1=ON (guard closed, battery on).
        //    PMDG only implements two states; BAT position is not available.
        steps_.push_back({
            .item          = "BATTERY switch",
            .requiredState = "GUARDED (closed)",
            .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.ELEC_BatSelector != 0; },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string { return d.ELEC_BatSelector != 0 ? "GUARDED (closed)" : "OFF"; },
            .execute       = [send](HANDLE hSC) { send(hSC, BatteryGuard, 1); },
            .verifications = {}
        });

        // 2. STANDBY POWER switch = GUARDED (closed)
        //    ELEC_StandbyPowerSelector: 0=BAT, 1=OFF, 2=AUTO.
        //    The guard holds the switch in the AUTO (normal) position.
        steps_.push_back({
            .item          = "STANDBY POWER switch",
            .requiredState = "GUARDED (closed) / AUTO",
            .isSatisfied   = [](const PMDG_NG3_Data& d) {
                return d.ELEC_StandbyPowerSelector == 2;
            },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                switch (d.ELEC_StandbyPowerSelector) {
                    case 0:  return "BAT";
                    case 1:  return "OFF";
                    default: return "GUARDED (closed) / AUTO";
                }
            },
            .execute = [send](HANDLE hSC) {
                send(hSC, StbyPwrSwitch, 2);  // set to AUTO
                send(hSC, StbyPwrGuard,  1);  // close the guard
            },
            .verifications = {}
        });

        // 3. ALTERNATE FLAPS master switch = GUARDED (closed)
        //    FCTL_AltnFlaps_Sw_ARM: true=ARM, false=OFF.
        //    The guard holds the switch in the OFF (normal/safe) position.
        steps_.push_back({
            .item          = "ALTERNATE FLAPS master switch",
            .requiredState = "GUARDED (closed) / OFF",
            .isSatisfied   = [](const PMDG_NG3_Data& d) {
                return !d.FCTL_AltnFlaps_Sw_ARM;
            },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                return d.FCTL_AltnFlaps_Sw_ARM ? "ARM" : "GUARDED (closed) / OFF";
            },
            .execute = [send](HANDLE hSC) {
                send(hSC, AltFlapsMasterSwitch, 0);  // set to OFF
                send(hSC, AltFlapsMasterGuard,  1);  // close the guard
            },
            .verifications = {}
        });

        // 4. WIPERS = PARK (both)
        //    OH_WiperLSelector / OH_WiperRSelector: 0=PARK, 1=INT, 2=LOW, 3=HIGH.
        steps_.push_back({
            .item          = "WIPERS",
            .requiredState = "PARK",
            .isSatisfied   = [](const PMDG_NG3_Data& d) {
                return d.OH_WiperLSelector == 0 && d.OH_WiperRSelector == 0;
            },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                static constexpr std::array<const char*, 4> positions{ "PARK", "INT", "LOW", "HIGH" };
                if (d.OH_WiperLSelector == 0 && d.OH_WiperRSelector == 0) return "PARK";
                const auto pos = [](BYTE v) -> const char* { return v < 4 ? positions[v] : "?"; };
                return std::format("L={} R={}", pos(d.OH_WiperLSelector), pos(d.OH_WiperRSelector));
            },
            .execute = [send](HANDLE hSC) {
                send(hSC, WiperLeft,  0);
                send(hSC, WiperRight, 0);
            },
            .verifications = {}
        });

        // 5. ELECTRIC HYD PUMPS = OFF (both)
        //    HYD_PumpSw_elec[0/1]: true=ON, false=OFF.
        steps_.push_back({
            .item          = "ELECTRIC HYD PUMPS",
            .requiredState = "OFF",
            .isSatisfied   = [](const PMDG_NG3_Data& d) {
                return !d.HYD_PumpSw_elec[0] && !d.HYD_PumpSw_elec[1];
            },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                if (!d.HYD_PumpSw_elec[0] && !d.HYD_PumpSw_elec[1]) return "OFF";
                return std::format("1={} 2={}", d.HYD_PumpSw_elec[0] ? "ON" : "OFF",
                                               d.HYD_PumpSw_elec[1] ? "ON" : "OFF");
            },
            .execute = [send](HANDLE hSC) {
                send(hSC, ElecHydPump1, 0);
                send(hSC, ElecHydPump2, 0);
            },
            .verifications = {}
        });

        // 6. LANDING GEAR lever = DN
        //    MAIN_GearLever: 0=UP, 1=OFF, 2=DOWN.
        //    MAIN_annunGEAR_locked[0..2] = green lights (locked).
        //    MAIN_annunGEAR_transit[0..2] = red lights  (in transit).
        steps_.push_back({
            .item          = "LANDING GEAR lever",
            .requiredState = "DN",
            .isSatisfied   = [](const PMDG_NG3_Data& d) {
                return d.MAIN_GearLever == 2;
            },
            .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                switch (d.MAIN_GearLever) {
                    case 0:  return "UP";
                    case 1:  return "OFF";
                    default: return "DN";
                }
            },
            .execute = [send](HANDLE hSC) {
                send(hSC, GearLever, 2);
            },
            .verifications = {
                {
                    "Gear green lights ON (all three locked)",
                    [](const PMDG_NG3_Data& d) {
                        return d.MAIN_annunGEAR_locked[0]
                            && d.MAIN_annunGEAR_locked[1]
                            && d.MAIN_annunGEAR_locked[2];
                    }
                },
                {
                    "Gear red lights OFF (no gear in transit)",
                    [](const PMDG_NG3_Data& d) {
                        return !d.MAIN_annunGEAR_transit[0]
                            && !d.MAIN_annunGEAR_transit[1]
                            && !d.MAIN_annunGEAR_transit[2];
                    }
                }
            }
        });

        // Path-specific steps: ground power or APU.
        if (powerSource_ == PowerSource::Ground) {
            // 7. Ground power available (wait/check — no automated action)
            steps_.push_back({
                .item          = "GRD PWR available",
                .requiredState = "AVAILABLE",
                .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.ELEC_annunGRD_POWER_AVAILABLE; },
                .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                    return d.ELEC_annunGRD_POWER_AVAILABLE ? "AVAILABLE" : "NOT AVAILABLE";
                },
                .execute       = nullptr,
                .verifications = {}
            });

            // 8. GRD PWR switch → ON
            steps_.push_back({
                .item          = "GRD PWR switch",
                .requiredState = "ON",
                .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.ELEC_GrdPwrSw; },
                .currentState  = [](const PMDG_NG3_Data& d) -> std::string {
                    return d.ELEC_GrdPwrSw ? "ON" : "OFF";
                },
                .execute = [send](HANDLE hSC) { send(hSC, GrdPwrSwitch, 1); },
                .verifications = {}
            });
        }
        else if (!skipFireTests_) {
            // APU path: verify all three fire handles are stowed (IN).
            // FIRE_HandlePos[0]=ENG1, [1]=APU, [2]=ENG2.  0=In, 1=Blocked, 2=Out.
            // These are physical handles — no automated action available.
            static constexpr std::array<const char*, 5> handlePos{
                "IN", "BLOCKED", "OUT", "TURNED LEFT", "TURNED RIGHT"
            };
            const auto posStr = [](BYTE v) -> const char* {
                return v < handlePos.size() ? handlePos[v] : "?";
            };

            steps_.push_back({
                .item          = "ENG 1 fire handle",
                .requiredState = "IN",
                .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.FIRE_HandlePos[0] == 0; },
                .currentState  = [posStr](const PMDG_NG3_Data& d) -> std::string {
                    return posStr(d.FIRE_HandlePos[0]);
                },
                .execute       = nullptr,
                .verifications = {}
            });
            steps_.push_back({
                .item          = "APU fire handle",
                .requiredState = "IN",
                .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.FIRE_HandlePos[1] == 0; },
                .currentState  = [posStr](const PMDG_NG3_Data& d) -> std::string {
                    return posStr(d.FIRE_HandlePos[1]);
                },
                .execute       = nullptr,
                .verifications = {}
            });
            steps_.push_back({
                .item          = "ENG 2 fire handle",
                .requiredState = "IN",
                .isSatisfied   = [](const PMDG_NG3_Data& d) { return d.FIRE_HandlePos[2] == 0; },
                .currentState  = [posStr](const PMDG_NG3_Data& d) -> std::string {
                    return posStr(d.FIRE_HandlePos[2]);
                },
                .execute       = nullptr,
                .verifications = {}
            });
        }
    }
};


// =============================================================================
// Display
// =============================================================================

static void enableAnsiVT()
{
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode{ 0 };
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

static void printProcedureStatus(const ElectricalPowerUpProcedure& proc,
                                  const PMDG_NG3_Data&               data,
                                  PowerSource                        source)
{
    const char* srcLabel = (source == PowerSource::Ground) ? "Ground Power" : "APU";
    std::cout << ANSI_CLEAR
              << std::format("737 Supplementary Procedures — Electrical Power Up ({})\n", srcLabel)
              << std::string(52, '-') << "\n\n";

    const size_t pendingIdx = proc.walkThrough(data);

    for (size_t i = 0; i <= pendingIdx && i < proc.stepCount(); ++i) {
        const auto& s    = proc.step(i);
        const bool  done = (i < pendingIdx);

        if (done) {
            std::cout << std::format("{}  [+]  {:<32}  {}{}\n",
                ANSI_GREEN, s.item, s.requiredState, ANSI_RESET);

            for (const auto& check : s.verifications) {
                const bool pass = check.passes(data);
                std::cout << std::format("{}         {}  {}{}\n",
                    pass ? ANSI_GREEN : ANSI_RED,
                    pass ? "[+]" : "[!]",
                    check.description,
                    ANSI_RESET);
            }
        }
        else {
            std::cout << std::format("{}  [ ]  {:<32}  {}{}\n",
                ANSI_YELLOW, s.item, s.currentState(data), ANSI_RESET);
        }
    }

    std::cout << "\n";

    if (pendingIdx >= proc.stepCount()) {
        std::cout << std::format("{}  Electrical Power Up complete.{}\n",
            ANSI_GREEN, ANSI_RESET);
    }
    else {
        const auto& pending = proc.step(pendingIdx);
        std::cout << std::format("{}  Action required:  {}  ->  {}{}\n",
            ANSI_YELLOW, pending.item, pending.requiredState, ANSI_RESET);
    }

    std::cout.flush();
}


// =============================================================================
// SimConnect helpers
// =============================================================================

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

static bool subscribeToPmdgData()
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

    // One-shot request to get the current state immediately on connect.
    // PERIOD_ON_SET does not deliver an initial snapshot, so we need this separately.
    hr = SimConnect_RequestClientData(hSimConnect,
        PMDG_NG3_DATA_ID, REQ_PMDG_DATA_INITIAL, PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_ONCE, 0,
        0, 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to request initial PMDG data: 0x{:08X}]\n", hr);
        return false;
    }

    // Ongoing subscription: wake up only when PMDG writes new values.
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


// =============================================================================
// Message loop
// =============================================================================

static void runMessageLoop(ElectricalPowerUpProcedure& proc, PowerSource source, RunMode mode)
{
    if (mode == RunMode::Cycle) {
        std::cerr << "[Monitoring Electrical Power Up state. Press Ctrl+C to quit.]\n";
    }

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
                if (pCd->dwRequestID == REQ_PMDG_DATA ||
                    pCd->dwRequestID == REQ_PMDG_DATA_INITIAL)
                {
                    printProcedureStatus(proc,
                        *reinterpret_cast<const PMDG_NG3_Data*>(&pCd->dwData),
                        source);
                    if (mode == RunMode::Once) {
                        return;
                    }
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


// =============================================================================
// Entry point
// =============================================================================

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    enableAnsiVT();

    PowerSource source{};
    bool        sourceSet{ false };
    RunMode     mode{ RunMode::Once };
    bool        skipFireTests{ false };
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{ argv[i] };
        if      (arg == "--gnd")            { source = PowerSource::Ground; sourceSet = true; }
        else if (arg == "--apu")            { source = PowerSource::APU;    sourceSet = true; }
        else if (arg == "--cycle")          { mode = RunMode::Cycle; }
        else if (arg == "--skipFireTests")  { skipFireTests = true; }
    }
    if (!sourceSet) {
        std::cerr << "Usage: " << argv[0] << " --gnd | --apu [--cycle] [--skipFireTests]\n";
        return 1;
    }

    if (!connect()) {
        return 1;
    }
    std::cerr << "[Connected to Flight Simulator.]\n";

    ElectricalPowerUpProcedure procedure{ PROC_EVENT_BASE, source, skipFireTests };
    procedure.registerEvents(hSimConnect);

    if (!subscribeToPmdgData()) {
        disconnect();
        return 1;
    }

    runMessageLoop(procedure, source, mode);
    disconnect();
    return 0;
}
