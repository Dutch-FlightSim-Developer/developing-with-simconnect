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

#include <array>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include <chrono>


#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/requests/simobject_and_livery_handler.hpp>

#include <simconnect/util/console_logger.hpp>
#include <simconnect/util/logger.hpp>

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
 * Gather command-line arguments into the args map.
 *
 * All commandline arguments starting with '--' are treated as flags and key-value pairs.
 * The other arguments are treated as positional arguments with keys 'Arg0', 'Arg1', etc.
 * Entry "Arg0" is always the program name.
 *
 * @param args The map to store the gathered arguments.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 */
static std::map<std::string, std::string> gatherArgs(int argc,
  const char *argv[])// NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
{
  std::map<std::string, std::string> args;
  int fixedArg{ 0 };

  args["Arg" + std::to_string(fixedArg++)] = argv[0];// NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];// NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (arg.starts_with("--")) {
      auto eqPos = arg.find('=');
      if (eqPos != std::string::npos) {
        const std::string key = arg.substr(2, eqPos - 2);
        const std::string value = arg.substr(eqPos + 1);

        args[key] = value;
      } else {
        args[arg.substr(2)] = "";// No value provided
      }
    } else {
      args["Arg" + std::to_string(fixedArg++)] = arg;
    }
  }
  return args;
}


auto main(int argc, const char *argv[]) -> int// NOLINT(bugprone-exception-escape)
{

    static constexpr const char *appName = "List titles and liveries";

    auto args = gatherArgs(argc, argv);

    SimObjectType simObjectType{ SimObjectTypes::aircraft };

    if (args.contains("Arg1")) {
        auto typeName = args["Arg1"];

        if (typeName == "user") {
            simObjectType = SimObjectTypes::user;
        }
        else if (typeName == "user-aircraft") {
            simObjectType = SimObjectTypes::userAircraft;
        }
        else if (typeName == "all") {
            simObjectType = SimObjectTypes::all;
        }
        else if (typeName == "aircraft") {
            simObjectType = SimObjectTypes::aircraft;
        }
        else if (typeName == "helicopter") {
            simObjectType = SimObjectTypes::helicopter;
        }
        else if (typeName == "boat") {
            simObjectType = SimObjectTypes::boat;
        }
        else if (typeName == "ground") {
            simObjectType = SimObjectTypes::ground;
        }
#if MSFS_2024_SDK
        else if (typeName == "balloon") {
            simObjectType = SimObjectTypes::hotAirBalloon;
        }
        else if (typeName == "animal") {
            simObjectType = SimObjectTypes::animal;
        }
        else if ((typeName == "user-avatar") || (typeName == "avatar")) {
            simObjectType = SimObjectTypes::userAvatar;
        }
        else if ((typeName == "user-current") || (typeName == "current")) {
            simObjectType = SimObjectTypes::userCurrent;
        }
#endif
        else {
            std::cerr << std::format("Unknown object type '{}'. Valid types are: user, user-aircraft, all, aircraft, helicopter, boat, ground, balloon, animal, user-avatar, avatar, user-current, current.\n",
                typeName);
            return -1;
        }
    }

    LogLevel logLevel{ LogLevel::Info };
    if (args.contains("debug")) {
        logLevel = LogLevel::Debug;
    }

    // Connect to the simulator
    WindowsEventConnection<true, ConsoleLogger> connection(appName);
    connection.logger().level(logLevel);
    WindowsEventHandler<true, ConsoleLogger> connectionHandler(connection);
    connectionHandler.logger().level(logLevel);

    connectionHandler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    connectionHandler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    connectionHandler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (!connection.open()) {
        std::cerr << "[ABORTING: Failed to connect to the simulator]\n";
        return 1;
    }

    SimObjectAndLiveryHandler<WindowsEventHandler<true, ConsoleLogger>> handler(connectionHandler);
    handler.logger().level(logLevel);
    bool listingDone{ false };

    auto request = handler.requestEnumeration(simObjectType, [&listingDone](const std::map<std::string, std::set<std::string>> &data) {
        std::cout << "Received enumeration of " << data.size() << " titles.\n";
        for (const auto &[title, liveries] : data) {
        std::cout << "Title: " << title << " has " << liveries.size() << " livery(ies):\n";
        for (const auto &livery : liveries) {
            std::cout << "  Livery: " << livery << '\n';
        }
        }
        listingDone = true;
    });

    static constexpr auto timeout = 30s;
    std::cerr << std::format("Listing liveries, will timeout after {} seconds...\n", timeout.count());
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();

    connection.close();

    return 0;
}