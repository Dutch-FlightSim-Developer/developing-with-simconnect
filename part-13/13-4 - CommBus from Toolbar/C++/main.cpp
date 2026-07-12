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


// 13-4 - CommBus from Toolbar - C++ listener. See ../README.md for the full picture.
//
// Run this from a terminal while a flight is loaded, then click the toolbar panel's
// "CommBus Sender" icon and click Send:
//
//   13-4-CommBusFromToolbar.exe

#include <exception>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <format>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/comm_bus_handler.hpp>

#include <simconnect/util/console_logger.hpp>


using namespace SimConnect;


#if MSFS_2024_SDK == 0
#error "The CommBus API requires the MSFS 2024 SDK. Use the 'Debug (MSFS 2024)' or 'Release (MSFS 2024)' configuration."
#endif


constexpr static const char* COMMBUS_HELLOJSON_EVENT = "DutchFlightSim.Tutorial.HelloJson";


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
static void handleOpen(const Messages::OpenMsg &msg)
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
static void handleClose([[maybe_unused]] const Messages::QuitMsg &msg) {
  std::cout << "Simulator shutting down.\n";
}


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
[[maybe_unused]]
static void handleException(const Messages::ExceptionMsg &msg)
{
  std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
  if (msg.dwSendID != unknownSendId) {
    std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
  }
  if (msg.dwIndex != Exceptions::unknownIndex) { std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex); }

  const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
  switch (exc) {
  case Exceptions::none: // Should never happen
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
  case Exceptions::weatherInvalidPort: // Legacy
    std::cerr << "The weather port is invalid.\n";
    break;
  case Exceptions::weatherInvalidMetar: // Legacy
    std::cerr << "The METAR string is invalid.\n";
    break;
  case Exceptions::weatherUnableToGetObservation: // Legacy
    std::cerr << "Unable to get the observation.\n";
    break;
  case Exceptions::weatherUnableToCreateStation: // Legacy
    std::cerr << "Unable to create the station.\n";
    break;
  case Exceptions::weatherUnableToRemoveStation: // Legacy
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
  case Exceptions::internal:
    std::cerr << "An internal SimConnect error has occurred.\n";
    break;
  default:
    std::cerr << std::format("An unknown exception code was received: {}.\n", msg.dwException);
    break;
  }
}


/**
 * Parse and print a "DutchFlightSim.Tutorial.HelloJson" payload.
 *
 * Expected shape: {"message": string, "flag": bool}. Malformed or unexpected payloads are
 * reported and otherwise ignored - this is a demo listener, not a validating one.
 *
 * @note Minimal and non-validating, written for this demo's fixed, flat payload shape only -
 * assumes "message" precedes "flag" and neither field contains escaped quotes or nested
 * structures. A general-purpose JSON parser isn't warranted for a two-field demo payload;
 * the MSFS SDK's bundled RapidJSON was tried first but doesn't compile under this project's
 * strict warnings-as-errors C++20 settings (a real const-member assignment in its own
 * headers, not just a suppressible warning).
 *
 * @param payload The reassembled CommBus payload (see CommBusHandler::subscribeToEvent).
 */
static void handleHelloJson(std::string_view payload)
{
  static const std::regex pattern{ R"re("message"\s*:\s*"([^"]*)"\s*,\s*"flag"\s*:\s*(true|false))re" };

  std::match_results<std::string_view::const_iterator> match;
  if (!std::regex_search(payload.cbegin(), payload.cend(), match, pattern)) {
    std::cerr << std::format("[Failed to parse JSON payload: '{}']\n", payload);
    return;
  }

  std::cout << std::format("Received: message='{}', flag={}\n", match[1].str(), match[2].str() == "true"); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
}


void runTest()
{
  WindowsEventConnection<false, ConsoleLogger> connection;
  WindowsEventHandler<false, ConsoleLogger> handler(connection);
  handler.autoClosing(true);

  handler.registerDefaultHandler([](const Messages::MsgBase& msg) {
    std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
  });
  handler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
  handler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
  handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

  std::cout << "Connecting to simulator...\n";
  if (connection.open()) {
    CommBusHandler commBusHandler(handler);
    auto subscription = commBusHandler.subscribeToEvent(COMMBUS_HELLOJSON_EVENT, handleHelloJson);

    std::cout << std::format("Subscribed to '{}'. Waiting for messages...\n", COMMBUS_HELLOJSON_EVENT);
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
