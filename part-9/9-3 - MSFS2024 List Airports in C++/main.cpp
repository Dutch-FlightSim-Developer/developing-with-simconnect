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

#include <format>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

#include <chrono>


#include <simconnect/simconnect.hpp>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/requests/request.hpp>
#include <simconnect/requests/facility_list_handler.hpp>

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
 * Convert a FacilityListType to a human-readable string.
 */
static std::string facilityTypeName(FacilityListType type)
{
  if (type == FacilityListTypes::airport) { return "airport"; }
  if (type == FacilityListTypes::waypoint) { return "waypoint"; }
  if (type == FacilityListTypes::ndb) { return "NDB"; }
  return "VOR";
}

/**
 * Convert a FacilitiesListScope to a human-readable string.
 */
static std::string scopeName(FacilitiesListScope scope)
{
  if (scope == FacilitiesListScope::allFacilities) { return "all"; }
  if (scope == FacilitiesListScope::bubbleOnly) { return "bubble"; }
  return "cache";
}

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
  case Exceptions::internal:
    std::cerr << "An internal SimConnect error has occurred.\n";
    break;
  default:
    std::cerr << std::format("An unknown exception code was received: {}.\n", msg.dwException);
    break;
  }
}


/**
 * Print airport information.
 * 
 * @param ident The airport identifier.
 * @param region The region code.
 * @param details The airport details.
 */
static void printAirport(std::string_view ident, std::string_view region, const AirportDetails& details)
{
  std::cout << std::format("Airport ID: '{}', Region: '{}', LatLonAlt: {:.2f}/{:.2f}/{:.2f}m\n",
    ident, region, details.position.Latitude, details.position.Longitude, details.position.Altitude);
}


/**
 * Print waypoint information.
 * 
 * @param ident The waypoint identifier.
 * @param region The region code.
 * @param details The waypoint details.
 */
static void printWaypoint(std::string_view ident, std::string_view region, const WaypointDetails& details)
{
  std::cout << std::format("Waypoint ID: '{}', Region: '{}', LatLonAlt: {:.2f}/{:.2f}/{:.2f}m, Magnetic variation: {:.2f}\n",
    ident, region, details.position.Latitude, details.position.Longitude, details.position.Altitude, details.magVar);
}


/**
 * Print NDB information.
 * 
 * @param ident The NDB identifier.
 * @param region The region code.
 * @param details The NDB details.
 */
static void printNdb(std::string_view ident, std::string_view region, const NdbDetails& details)
{
  static constexpr double kHzFactor = 1.0;
  std::cout << std::format("NDB ID: '{}', Region: '{}', Frequency: {:06.2f} kHz, LatLonAlt: {:.2f}/{:.2f}/{:.2f}m, Magnetic variation: {:.2f}\n",
    ident, region, details.frequency / kHzFactor, details.position.Latitude, details.position.Longitude, details.position.Altitude, details.magVar);
}


/**
 * Print VOR information.
 * 
 * @param ident The VOR identifier.
 * @param region The region code.
 * @param details The VOR details.
 */
static void printVor(std::string_view ident, std::string_view region, const VorDetails& details)
{
  static constexpr double MHzFactor = 1.0;
  std::cout << std::format("VOR ID: '{}', Region: '{}', Frequency: {:06.2f} MHz, LatLonAlt: {:.2f}/{:.2f}/{:.2f}m, Magnetic variation: {:.2f}",
    ident, region, details.frequency / MHzFactor, details.position.Latitude, details.position.Longitude, details.position.Altitude, details.magVar);
  
  std::cout << ", Capabilities: [";
  if (details.hasNavSignal()) {
    std::cout << "NAV";
  }
  if (details.hasDME()) {
    std::cout << (details.hasNavSignal() ? "+" : "") << "DME";
  }
  if (details.hasLocalizer()) {
    std::cout << (details.hasNavSignal() || details.hasDME() ? "+" : "") << "LOC";
    std::cout << std::format(" (course: {:.2f}°, pos: {:.2f}/{:.2f}/{:.2f}m)", 
      details.localizerCourse, details.localizerPosition.Latitude, details.localizerPosition.Longitude, details.localizerPosition.Altitude);
  }
  if (details.hasGlideSlope()) {
    std::cout << std::format("+GS (angle: {:.2f}°)", details.glideSlopeAngle);
  }
  std::cout << "]\n";
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


auto main(int argc, const char *argv[]) -> int// NOLINT(bugprone-exception-escape,readability-function-cognitive-complexity)
{

    static constexpr const char *appName = "List titles and liveries";

    auto args = gatherArgs(argc, argv);

	FacilitiesListScope queryType{ FacilitiesListScope::cacheOnly };

    if (args.contains("bubble")) {
        queryType = FacilitiesListScope::bubbleOnly;
    } else if (args.contains("all")) {
        queryType = FacilitiesListScope::allFacilities;
    } else if (args.contains("cache")) { // Default actually...
        queryType = FacilitiesListScope::cacheOnly;
    }

    FacilityListType facilityType{ FacilityListTypes::airport };
    if (args.contains("Arg1")) {
        const std::string arg1 = args["Arg1"];
        if (arg1 == "airport") {
            facilityType = FacilityListTypes::airport;
        } else if (arg1 == "waypoint") {
            facilityType = FacilityListTypes::waypoint;
        } else if (arg1 == "ndb") {
            facilityType = FacilityListTypes::ndb;
        } else if (arg1 == "vor") {
            facilityType = FacilityListTypes::vor;
        } else {
            std::cerr << "Invalid first argument.\n"
                      << "Usage: " << args["Arg0"] << " [airport|waypoint|ndb|vor] [--all|--bubble|--cache] [--ident=ID] [--region=REGION]\n";
            return -1;
        }
    }

    std::string identFilter = args.contains("ident") ? args["ident"] : "";
    std::string regionFilter = args.contains("region") ? args["region"] : "";

    std::cout << std::format("Requesting list of {}s using {} data", facilityTypeName(facilityType), scopeName(queryType));
    if (!identFilter.empty()) {
        std::cout << std::format(" with ident='{}'", identFilter);
    }
    if (!regionFilter.empty()) {
        std::cout << std::format(" in region='{}'", regionFilter);
    }
    std::cout << "...\n";

    // Connect to the simulator
    WindowsEventConnection<true, ConsoleLogger> connection(appName);
    connection.logger().level(LogLevel::Debug);
    WindowsEventHandler<true, ConsoleLogger> connectionHandler(connection);

    connectionHandler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    connectionHandler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    connectionHandler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (!connection.open()) {
        std::cerr << "[ABORTING: Failed to connect to the simulator]\n";
        return 1;
    }
    FacilityListHandler<decltype(connectionHandler)> handler(connectionHandler);
    bool listingDone{ false };
    unsigned long count{ 0 };

    Request request{ 0, nullptr };

    if (facilityType == FacilityListTypes::airport) {
        request = handler.listAirports(queryType,
            [&count, &identFilter, &regionFilter](std::string_view ident, std::string_view region, const AirportDetails& details) {
                if ((!identFilter.empty() && ident.find(identFilter) == std::string_view::npos) ||
                    (!regionFilter.empty() && region.find(regionFilter) == std::string_view::npos)) {
                    return;
                }
                printAirport(ident, region, details);
                count++;
            },
            [&listingDone, &count]() {
                std::cout << std::format("\nTotal airports: {}\n", count);
                listingDone = true;
            });
    } else if (facilityType == FacilityListTypes::waypoint) {
        request = handler.listWaypoints(queryType,
            [&count, &identFilter, &regionFilter](std::string_view ident, std::string_view region, const WaypointDetails& details) {
                if ((!identFilter.empty() && ident.find(identFilter) == std::string_view::npos) ||
                    (!regionFilter.empty() && region.find(regionFilter) == std::string_view::npos)) {
                    return;
                }
                printWaypoint(ident, region, details);
                count++;
            },
            [&listingDone, &count]() {
                std::cout << std::format("\nTotal waypoints: {}\n", count);
                listingDone = true;
            });
    } else if (facilityType == FacilityListTypes::ndb) {
        request = handler.listNDBs(queryType,
            [&count, &identFilter, &regionFilter](std::string_view ident, std::string_view region, const NdbDetails& details) {
                if ((!identFilter.empty() && ident.find(identFilter) == std::string_view::npos) ||
                    (!regionFilter.empty() && region.find(regionFilter) == std::string_view::npos)) {
                    return;
                }
                printNdb(ident, region, details);
                count++;
            },
            [&listingDone, &count]() {
                std::cout << std::format("\nTotal NDBs: {}\n", count);
                listingDone = true;
            });
    } else if (facilityType == FacilityListTypes::vor) {
        request = handler.listVORs(queryType,
            [&count, &identFilter, &regionFilter](std::string_view ident, std::string_view region, const VorDetails& details) {
                if ((!identFilter.empty() && ident.find(identFilter) == std::string_view::npos) ||
                    (!regionFilter.empty() && region.find(regionFilter) == std::string_view::npos)) {
                    return;
                }
                printVor(ident, region, details);
                count++;
            },
            [&listingDone, &count]() {
                std::cout << std::format("\nTotal VORs: {}\n", count);
                listingDone = true;
            });
    }

    static constexpr auto timeout = 30s;
    std::cout << std::format("Listing facilities, will timeout after {} seconds...\n", timeout.count());
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();

    connection.close();

    return 0;
}