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


// PMDG 737 NG3 - Selective MCP Monitor (stateless C++ version)
//
// Subscribes to the PMDG_NG3_Data client data area but defines ONLY the
// six MCP display fields, at their exact byte offsets within PMDG_NG3_Data.
//
// PMDG sends the full PMDG_NG3_Data block every time anything in the aircraft
// changes.  Because we requested tagged + onlyWhenChanged, SimConnect delivers
// only the datums that actually changed on each write.  The per-datum callbacks
// fire independently — a fuel-pump toggle produces no output here at all.
//
// Key points demonstrated:
//   1. Receiver assigns its own datum IDs (1..7) via StatelessClientDataDefinition.
//      PMDG's own datum IDs are completely irrelevant on the receiver side.
//   2. The explicit offset parameter (offsetof) tells SimConnect which bytes in
//      PMDG's block correspond to each of our datums.
//   3. No PMDG_NG3_Data variable is stored — only tiny scalars needed to combine
//      the IAS/VS values with their blank flags.
//   4. Changing a non-MCP field in the aircraft does NOT trigger any callback here.
//
// Prerequisites
//   - MSFS running with the PMDG 737 loaded.
//   - [SDK] EnableDataBroadcast=1 in 737NG3_Options.ini
//     (typically found in the PMDG work folder inside your Community folder).

#pragma warning(push, 3)
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <string>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/requests/client_data_handler.hpp>
#include <simconnect/data/stateless_client_data_definition.hpp>
#include <simconnect/data_frequency.hpp>

#include <simconnect/util/console_logger.hpp>


using namespace SimConnect;


/**
 * Return a pretty formatted version string.
 *
 * @param major Major version number. If 0, return "Unknown".
 * @param minor Minor version number. If 0, return just the major version number.
 * @return version string.
 */
static std::string version(unsigned long major, unsigned long minor)
{
    if (major == 0) { return "Unknown"; }
    return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


/**
 * Handle the SIMCONNECT_RECV_OPEN message.
 */
[[maybe_unused]]
static void handleOpen(const Messages::OpenMsg& msg)
{
    std::cout << "Connected to " << &(msg.szApplicationName[0]) << " version "
              << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << '\n'
              << "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << '\n'
              << "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor)
              << '\n'
              << "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << '\n';
}


/**
 * Handle the SIMCONNECT_RECV_QUIT message.
 */
[[maybe_unused]]
static void handleClose([[maybe_unused]] const Messages::QuitMsg& msg) { std::cout << "Simulator shutting down.\n"; }


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
[[maybe_unused]]
static void handleException(const Messages::ExceptionMsg& msg)
{
    std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
    if (msg.dwSendID != unknownSendId) {
        std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
    }
    if (msg.dwIndex != Exceptions::unknownIndex) { std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex); }

    const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
    switch (exc) {
    case Exceptions::none:
        std::cerr << "No exception.\n";
        break;
    case Exceptions::error:
        std::cerr << "Some unspecific error has occurred.\n";
        break;
    case Exceptions::sizeMismatch:
        std::cerr << "The size of the parameter does not match the expected size.\n";
        break;
    case Exceptions::unrecognizedId:
        std::cerr << "The parameter is not a recognized ID.\n";
        break;
    case Exceptions::unopened:
        std::cerr << "The connection has not been opened.\n";
        break;
    case Exceptions::versionMismatch:
        std::cerr << "This version of SimConnect cannot work with this version of the simulator.\n";
        break;
    case Exceptions::tooManyGroups:
        std::cerr << "The maximum number of (input/notification) groups has been reached. (currently 20)\n";
        break;
    case Exceptions::nameUnrecognized:
        std::cerr << "The parameter is not a recognized name.\n";
        break;
    case Exceptions::tooManyEventNames:
        std::cerr << "The maximum number of event names has been reached. (currently 1000)\n";
        break;
    case Exceptions::eventIdDuplicate:
        std::cerr << "The event ID is already in use.\n";
        break;
    case Exceptions::tooManyMaps:
        std::cerr << "The maximum number of mapings has been reached. (currently 20)\n";
        break;
    case Exceptions::tooManyObjects:
        std::cerr << "The maximum number of objects has been reached. (currently 1000)\n";
        break;
    case Exceptions::tooManyRequests:
        std::cerr << "The maximum number of requests has been reached. (currently 1000)\n";
        break;
    case Exceptions::weatherInvalidPort:
        std::cerr << "The weather port is invalid.\n";
        break;
    case Exceptions::weatherInvalidMetar:
        std::cerr << "The METAR string is invalid.\n";
        break;
    case Exceptions::weatherUnableToGetObservation:
        std::cerr << "Unable to get the observation.\n";
        break;
    case Exceptions::weatherUnableToCreateStation:
        std::cerr << "Unable to create the station.\n";
        break;
    case Exceptions::weatherUnableToRemoveStation:
        std::cerr << "Unable to remove the station.\n";
        break;
    case Exceptions::invalidDataType:
        std::cerr << "The requested data cannot be converted to the specified data type.\n";
        break;
    case Exceptions::invalidDataSize:
        std::cerr << "The requested data cannot be transferred in the specified data size.\n";
        break;
    case Exceptions::dataError:
        std::cerr << "The data passed is invalid.\n";
        break;
    case Exceptions::invalidArray:
        std::cerr << "The array passed to SetDataOnSimObject is invalid.\n";
        break;
    case Exceptions::createObjectFailed:
        std::cerr << "The AI object could not be created.\n";
        break;
    case Exceptions::loadFlightplanFailed:
        std::cerr << "The flight plan could not be loaded. Either it could not be found, or it contained an error.\n";
        break;
    case Exceptions::operationInvalidForObjectType:
        std::cerr << "The operation is not valid for the object type.\n";
        break;
    case Exceptions::illegalOperation:
        std::cerr << "The operation is illegal. (AI or Weather)\n";
        break;
    case Exceptions::alreadySubscribed:
        std::cerr << "The client is already subscribed to this event.\n";
        break;
    case Exceptions::invalidEnum:
        std::cerr << "The type enum value is unknown. (Probably an unknown type in RequestDataOnSimObjectType)\n";
        break;
    case Exceptions::definitionError:
        std::cerr << "The definition is invalid. (Probably a variable length requested in RequestDataOnSimObject)\n";
        break;
    case Exceptions::duplicateId:
        std::cerr << "The ID is already in use. (Menu, DataDefinition item ID, ClientData mapping, or event to "
                     "notification group)\n";
        break;
    case Exceptions::datumId:
        std::cerr << "Unknown datum ID specified for SetDataOnSimObject.\n";
        break;
    case Exceptions::outOfBounds:
        std::cerr
            << "The requested value is out of bounds. (radius of a RequestDataOnSimObjectType, or CreateClientData)\n";
        break;
    case Exceptions::alreadyCreated:
        std::cerr << "A ClientData area with that name has already been created.\n";
        break;
    case Exceptions::objectOutsideRealityBubble:
        std::cerr << "The AI object is outside the reality bubble.\n";
        break;
    case Exceptions::objectContainer:
        std::cerr << "The AI object creation failed. (container issue)\n";
        break;
    case Exceptions::objectAi:
        std::cerr << "The AI object creation failed. (AI issue)\n";
        break;
    case Exceptions::objectAtc:
        std::cerr << "The AI object creation failed. (ATC issue)\n";
        break;
    case Exceptions::objectSchedule:
        std::cerr << "The AI object creation failed. (scheduling issue)\n";
        break;
    case Exceptions::jetwayData:
        std::cerr << "Requesting JetWay data failed.\n";
        break;
    case Exceptions::actionNotFound:
        std::cerr << "The action was not found.\n";
        break;
    case Exceptions::notAnAction:
        std::cerr << "The action was not a valid action.\n";
        break;
    case Exceptions::incorrectActionParams:
        std::cerr << "The action parameters were incorrect.\n";
        break;
    case Exceptions::getInputEventFailed:
        std::cerr << "The input event name was not found. (GetInputEvent)\n";
        break;
    case Exceptions::setInputEventFailed:
        std::cerr << "The input event name was not found. (SetInputEvent)\n";
        break;
#if MSFS_2024_SDK
    case Exceptions::internal:
        std::cerr << "An internal SimConnect error has occurred.\n";
        break;
#endif
    default:
        std::cerr << std::format("An unknown exception code was received: {}.\n", msg.dwException);
        break;
    }
}


void runTest()
{
    WindowsEventConnection<false, ConsoleLogger> connection;
    WindowsEventHandler<false, ConsoleLogger> handler(connection);
    ClientDataHandler<WindowsEventHandler<false, ConsoleLogger>> dataHandler(handler);
    handler.autoClosing(true);

    handler.registerDefaultHandler([](const Messages::MsgBase& msg) {
        std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
    });
    handler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    handler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (connection.open()) {
        std::cerr << "[Monitoring MCP displays selectively. Press Ctrl+C to quit.]\n\n";

        // Map PMDG's published name — PMDG owns this area, we are a read-only subscriber.
        const ClientDataId dataId = dataHandler.mapClientDataName(PMDG_NG3_DATA_NAME);

        // Minimal state for combining IAS value with its blank flag, and VS with its blank flag.
        float lastIas{ 0.0F };
        bool  lastIasBlank{ true };
        short lastVs{ 0 };
        bool  lastVsBlank{ true };
        static constexpr float maxMach{ 10.0F }; // just for sanity-checking the IAS/Mach value before printing

        // Define only the seven MCP fields we care about, pinned to their exact positions
        // inside PMDG_NG3_Data via offsetof(). Datum IDs 1..7 are ours — PMDG's own IDs
        // (if any) play no role on the subscriber side.
        StatelessClientDataDefinition def;
        def
          .addInt8(
              [](int8_t v) { std::cout << std::format("[MCP] Power:    {}\n", v ? "ON" : "OFF"); },
              0.0F, offsetof(PMDG_NG3_Data, MCP_indication_powered))
          .addFloat32(
              [&lastIas, &lastIasBlank](float v) {
                  lastIas = v;
                  if (lastIasBlank) { return; }
                  if (lastIas < maxMach) { std::cout << std::format("[MCP] Speed:    M{:.2f}\n", lastIas); }
                  else                 { std::cout << std::format("[MCP] Speed:    {:.0f} kts\n", lastIas); }
              },
              0.0F, offsetof(PMDG_NG3_Data, MCP_IASMach))
          .addInt8(
              [&lastIas, &lastIasBlank](int8_t v) {
                  lastIasBlank = (v != 0);
                  if (lastIasBlank)         { std::cout << "[MCP] Speed:    ---\n"; }
                  else if (lastIas < maxMach) { std::cout << std::format("[MCP] Speed:    M{:.2f}\n", lastIas); }
                  else                      { std::cout << std::format("[MCP] Speed:    {:.0f} kts\n", lastIas); }
              },
              0.0F, offsetof(PMDG_NG3_Data, MCP_IASBlank))
          .addInt16(
              [](int16_t v) { std::cout << std::format("[MCP] Heading:  {:03}\n", static_cast<unsigned short>(v)); },
              0.0F, offsetof(PMDG_NG3_Data, MCP_Heading))
          .addInt16(
              [](int16_t v) { std::cout << std::format("[MCP] Altitude: {} ft\n", static_cast<unsigned short>(v)); },
              0.0F, offsetof(PMDG_NG3_Data, MCP_Altitude))
          .addInt16(
              [&lastVs, &lastVsBlank](int16_t v) {
                  lastVs = v;
                  if (!lastVsBlank) { std::cout << std::format("[MCP] VS:       {:+} fpm\n", static_cast<int>(lastVs)); }
              },
              0.0F, offsetof(PMDG_NG3_Data, MCP_VertSpeed))
          .addInt8(
              [&lastVs, &lastVsBlank](int8_t v) {
                  lastVsBlank = (v != 0);
                  if (lastVsBlank) { std::cout << "[MCP] VS:       ---\n"; }
                  else             { std::cout << std::format("[MCP] VS:       {:+} fpm\n", static_cast<int>(lastVs)); }
              },
              0.0F, offsetof(PMDG_NG3_Data, MCP_VertSpeedBlank));

        // Tagged + onlyWhenChanged: SimConnect delivers only the datums that actually changed
        // since the last PMDG write — keyed by OUR datum IDs, not PMDG's.
        auto req = dataHandler.requestClientDataTagged(
            dataId, def,
            ClientDataFrequency::onSet(),
            PeriodLimits::none(),
            /*onlyWhenChanged=*/ true);

        handler.handleUntilClosed();
    } else {
        std::cerr << "Failed to connect to simulator.\n";
    }
}


auto main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    try {
        runTest();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
