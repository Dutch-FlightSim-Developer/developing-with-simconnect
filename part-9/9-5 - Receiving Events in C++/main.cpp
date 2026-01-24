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


#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <chrono>
#include <span>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>

#include <simconnect/util/console_logger.hpp>
#include <simconnect/util/logger.hpp>
#include <simconnect/util/args.hpp>

#include <simconnect/windows_event_connection.hpp>
using ThisConnection = SimConnect::WindowsEventConnection<true, SimConnect::ConsoleLogger>;
#include <simconnect/windows_event_handler.hpp>
using ThisConnectionHandler = SimConnect::WindowsEventHandler<true, SimConnect::ConsoleLogger>;

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/input_group.hpp>
#include <simconnect/events/notification_group.hpp>


using namespace SimConnect;
using namespace std::chrono_literals;


/**
 * Return a pretty formatted version string.
 * @param major major version number. If 0, return "Unknown".
 * @param minor minor version number. If 0, return just the major version number.
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
static void handleClose([[maybe_unused]] const Messages::QuitMsg &msg) { std::cout << "Simulator shutting down.\n"; }


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
static void handleException(const Messages::ExceptionMsg &msg)
{

  std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
  if (msg.dwSendID != unknownSendId) {
    std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
  }
  if (msg.dwIndex != Exceptions::unknownIndex) { std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex); }

  const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
  switch (exc) {
  case Exceptions::none:// Should never happen
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
  case Exceptions::weatherInvalidPort:// Legacy
    std::cerr << "The weather port is invalid.\n";
    break;
  case Exceptions::weatherInvalidMetar:// Legacy
    std::cerr << "The METAR string is invalid.\n";
    break;
  case Exceptions::weatherUnableToGetObservation:// Legacy
    std::cerr << "Unable to get the observation.\n";
    break;
  case Exceptions::weatherUnableToCreateStation:// Legacy
    std::cerr << "Unable to create the station.\n";
    break;
  case Exceptions::weatherUnableToRemoveStation:// Legacy
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


/**
 * Set up keyboard input to toggle recording and exit the program.
 */
template<typename EvtHandler, typename InputGroup, typename ExitHandler>
static bool setupKeys(EvtHandler &eventHandler, InputGroup &inputGroup, ExitHandler&& onExit)
{
  std::cerr << "[Press the Stop key to exit the program]\n";

  const event exit = event::get("Exit.Program");
  inputGroup.addEvent(exit, "VK_MEDIA_STOP");
  eventHandler.template registerEventHandler<Messages::EventMsg>(
    exit,
    [onExit = std::forward<ExitHandler>(onExit)]([[maybe_unused]] const Messages::EventMsg &evt) {
        std::cerr << "[Exit key pressed]\n";
        onExit();
    });

  return inputGroup;
}


constexpr static const char* appName = "SimConnect Console Application";


auto main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    const Util::Args args(std::span<const char*>(argv, argc));

    const bool debug = args.has("debug");
    const LogLevel logLevel = debug ? LogLevel::Debug : LogLevel::Info;

    // Connect to the simulator
    ThisConnection connection(appName);
    connection.logger().level(logLevel);
    ThisConnectionHandler connectionHandler(connection);
    connectionHandler.logger().level(logLevel);

    if (debug) {
        connectionHandler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    }
    connectionHandler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    connectionHandler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    std::cout << "Connecting to simulator...\n";
    if (!connection.open()) {
        std::cerr << "Failed to connect to simulator.\n";
        return 1;
    }
    std::cout << "Connected to simulator.\n";

    EventHandler<WindowsEventHandler<true, ConsoleLogger>> eventHandler(connectionHandler);

    auto inputGroup = eventHandler.createInputGroup().withHighestPriority().enable();
    // Set up keyboard input if requested
    if (!setupKeys(eventHandler, inputGroup, [&connection] { connection.close(); })) {
      std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
      return 1;
    }

    auto notificationGroup = eventHandler.createNotificationGroup().withStandardPriority();

    notificationGroup
        .addEvent(event::get("FLAPS_SET"))
        .addEvent(event::get("FLAPS_INCR"))
        .addEvent(event::get("FLAPS_DECR"))
        .addEvent(event::get("FLAPS_UP"))
        .addEvent(event::get("FLAPS_DOWN"))
        .addEvent(event::get("AXIS_FLAPS_SET"))
        .addEvent(event::get("FLAPS_1"))
        .addEvent(event::get("FLAPS_2"))
        .addEvent(event::get("FLAPS_3"))
        //.addEvent(event::get("FLAPS_4"))  // Not available in MSFS
        .addEvent(event::get("FLAPS_CONTINUOUS_SET"))
        .addEvent(event::get("FLAPS_CONTINUOUS_INCR"))
        .addEvent(event::get("FLAPS_CONTINUOUS_DECR"));

    eventHandler.registerEventGroupHandler<Messages::EventMsg>(notificationGroup,
        [](const Messages::EventMsg &evt) {
            std::cout << std::format("Received flap event '{}' (ID {})\n", event::get(evt.uEventID).name(), evt.uEventID);
        });

    const auto timeout = 30s;
    connectionHandler.handleFor(timeout);

    std::cout << "Disconnected from simulator. Exiting.\n";
	return 0;
}