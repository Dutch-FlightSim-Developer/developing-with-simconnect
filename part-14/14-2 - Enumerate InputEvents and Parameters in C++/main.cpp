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


#include <cstddef>
#include <exception>
#include <chrono>
#include <map>
#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <regex>
#include <vector>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/util/console_logger.hpp>

#include <simconnect/requests/input_event_handler.hpp>


using namespace SimConnect;
using namespace std::chrono_literals;


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
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return Map of gathered arguments.
 */
[[maybe_unused]]
static std::map<std::string, std::string> gatherArgs(int argc,
  const char *argv[]) // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
{
  std::map<std::string, std::string> args;
  int fixedArg{ 0 };

  args["Arg" + std::to_string(fixedArg++)] = argv[0]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (arg.starts_with("--")) {
      auto eqPos = arg.find('=');
      if (eqPos != std::string::npos) {
        const std::string key = arg.substr(2, eqPos - 2);
        const std::string value = arg.substr(eqPos + 1);

        args[key] = value;
      } else {
        args[arg.substr(2)] = ""; // No value provided
      }
    } else {
      args["Arg" + std::to_string(fixedArg++)] = arg;
    }
  }
  return args;
}


/**
 * Print a non-empty ';'-separated parameter type string in human-readable form.
 *
 * @param value The raw ';'-separated parameter type string. Must not be empty.
 */
static void printParams(std::string_view value)
{
  std::cout << "Parameters:\n";
  std::size_t start{ 0 };
  std::size_t end{ value.find(';') };
  while (end != std::string_view::npos) {
    const auto param{ value.substr(start, end - start) };
    if (!param.empty()) {
      std::cout << std::format("- {}\n", param);
    }
    start = end + 1;
    end = value.find(';', start);
  }
  if (start < value.length()) {
    const auto param{ value.substr(start) };
    if (!param.empty()) {
      std::cout << std::format("- {}\n", param);
    }
  }
}


/**
 * Print a single line describing a declared input event, without its parameters.
 *
 * @param event The event to print.
 */
static void printEvent(const InputEvent& event)
{
  std::cout << std::format("Event: '{}' (Hash: 0x{:016X}, Type: {})\n",
    event.name, event.hash, event.type == InputEventTypes::doubleValue ? "Double" : "String");
}


/**
 * List every declared input event matching `pattern` (a regex; empty matches everything). Unlike
 * the default (non-scan) mode, this always just lists - even a single match's parameters are not
 * fetched, since the point here is surveying what's declared, not looking up one known event.
 *
 * @param handler The message handler to pump while waiting for the enumeration to complete.
 * @param inputEvents The input event handler to enumerate through.
 * @param pattern The regex pattern to filter declared input event names by.
 * @param done Set to true once the enumeration completes (by this function) or an exception
 *             arrived (by the caller's exception handler) - either way, pumping should stop.
 * @return 0 if at least one event matched, 1 otherwise (no match, bad pattern).
 */
template <class Handler>
static int scanInputEvents(Handler& handler, InputEventHandler<Handler>& inputEvents, std::string_view pattern, bool& done)
{
  std::regex regexPattern;
  const bool havePattern{ !pattern.empty() };
  if (havePattern) {
    try {
      regexPattern = std::regex(pattern.begin(), pattern.end());
    }
    catch (const std::regex_error& e) {
      std::cerr << std::format("Invalid regex pattern '{}': {}\n", pattern, e.what());
      return 1;
    }
  }

  std::cout << std::format("[Scanning input events{}]\n", havePattern ? std::format(" matching '{}'", pattern) : "");

  int exitCode{ 1 };
  std::vector<InputEvent> matches;

  auto request = inputEvents.enumerateInputEvents(
    [&matches, &regexPattern, havePattern](InputEventHash hash, std::string_view name, InputEventType type) {
      if (!havePattern || std::regex_match(name.begin(), name.end(), regexPattern)) {
        matches.push_back(InputEvent{ .name = std::string(name), .hash = hash, .type = type });
      }
    },
    [&matches, &done, &exitCode]() {
      if (matches.empty()) {
        std::cerr << "No declared input event matches.\n";
      } else {
        for (const auto& event : matches) {
          printEvent(event);
        }
        exitCode = 0;
      }
      done = true;
    });

  static constexpr auto timeout = 10s;
  handler.handleUntilOrTimeout([&done]() { return done; }, timeout);
  request.stop();

  return exitCode;
}


/**
 * Look up a single declared input event by exact name and print its parameters - the common case
 * of a real add-on wiring up one known event, as opposed to scanInputEvents()'s survey.
 *
 * @param handler The message handler to pump while waiting for the lookup to complete.
 * @param inputEvents The input event handler to look the event up through.
 * @param name The exact (case-sensitive) name of the input event to find.
 * @param done Set to true once the lookup completes (by this function) or an exception arrived
 *             (by the caller's exception handler) - either way, pumping should stop.
 * @return 0 if found, 1 otherwise.
 */
template <class Handler>
static int lookupInputEvent(Handler& handler, InputEventHandler<Handler>& inputEvents, std::string_view name, bool& done)
{
  std::cout << std::format("[Looking up input event '{}']\n", name);

  int exitCode{ 1 };

  auto request = inputEvents.findInputEvent(name,
    [&done, &exitCode](const InputEvent& event, std::string_view value) {
      printEvent(event);
      if (value.empty()) {
        std::cout << std::format("No parameters found for event '{}'.\n", event.name);
      } else {
        printParams(value);
      }
      exitCode = 0;
      done = true;
    },
    [&done]() {
      std::cerr << "No declared input event with that name.\n";
      done = true;
    });

  static constexpr auto timeout = 10s;
  handler.handleUntilOrTimeout([&done]() { return done; }, timeout);
  request.stop();

  return exitCode;
}


/**
 * Connect to the simulator and either scan for input events matching a regex (--scan), or look
 * up a single event by exact name (the default).
 *
 * @param pattern The regex pattern (--scan) or exact name (default) to look up.
 * @param scan Whether to scan (list all matches) instead of looking up a single exact name.
 * @return 0 on a resolved match, 1 otherwise (no match, bad pattern, connection failure).
 */
static int runTest(std::string_view pattern, bool scan)
{
  WindowsEventConnection<false, ConsoleLogger> connection;
  WindowsEventHandler<false, ConsoleLogger> handler(connection);
  handler.autoClosing(true);

  bool done{ false };

  handler.registerDefaultHandler([](const Messages::MsgBase& msg) {
    std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
  });
  handler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
  handler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
  handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, [&done](const Messages::ExceptionMsg& msg) {
    handleException(msg);
    done = true;
  });

  if (!connection.open()) {
    std::cerr << "Failed to connect to simulator.\n";
    return 1;
  }

  InputEventHandler<decltype(handler)> inputEvents(handler);

  return scan ? scanInputEvents(handler, inputEvents, pattern, done) : lookupInputEvent(handler, inputEvents, pattern, done);
}


auto main(int argc, const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
  const auto args{ gatherArgs(argc, argv) };
  const auto nameArg{ args.find("Arg1") };
  const std::string pattern{ (nameArg != args.end()) ? nameArg->second : std::string{} };
  const bool scan{ args.contains("scan") };

  try {
    return runTest(pattern, scan);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
