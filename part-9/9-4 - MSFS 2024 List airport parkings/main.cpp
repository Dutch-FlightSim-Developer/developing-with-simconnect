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
#include <set>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>

#include <chrono>
#include <cstddef>

#include <simconnect/simconnect.hpp>

#include <simconnect/util/console_logger.hpp>
#include <simconnect/util/logger.hpp>

#include <simconnect/windows_event_connection.hpp>
using ThisConnection = SimConnect::WindowsEventConnection<true, SimConnect::ConsoleLogger>;
#include <simconnect/windows_event_handler.hpp>
using ThisSimConnectHandler = SimConnect::WindowsEventHandler<true, SimConnect::ConsoleLogger>;

#include <simconnect/requests/facility_handler.hpp>
#include <simconnect/requests/facility_list_handler.hpp>
#include <simconnect/requests/facilities/facility_definition.hpp>
#include <simconnect/requests/facilities/facility_definition_builder.hpp>
#include <simconnect/requests/facilities/airport.hpp>
#include <simconnect/requests/facilities/frequency.hpp>
#include <simconnect/requests/facilities/taxi_parking.hpp>
#include <simconnect/requests/facilities/vor.hpp>

using namespace SimConnect;
using namespace std::chrono_literals;


static constexpr std::string_view degreeSymbol = "\xF8";

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
  case Exceptions::internal:
    std::cerr << "An internal SimConnect error has occurred.\n";
    break;
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


/**
 * Print a single taxi parking entry.
 * 
 * @param parkingName The formatted name of the parking.
 */
static void printTaxiParking(const std::string& parkingName, const Facilities::TaxiParkingFacility& parking, const Facilities::AirportData& airportData)
{
    std::cout << std::format("Parking '{}': (Orientation {}, Heading {:03}) at Airport {} (Region {}) Lat {:.6f} Lon {:.6f} Alt {:.2f}m\n",
        parkingName,
        parking.data.isOrientationForward() ? "Forward" : "Reverse",
        static_cast<int>(parking.data.heading()),
        airportData.icao(),
        airportData.region(),
        parking.data.latitude(airportData.latitude(), airportData.longitude()),
        parking.data.longitude(airportData.latitude(), airportData.longitude()),
        airportData.altitude()
    );
}


/**
 * Get VOR capability flags.
 */
static std::set<std::string> vorFlags(const Facilities::VORData& vor)
{
    std::set<std::string> flags;

    if (vor.isNav()) { flags.insert("NAV"); }
    if (vor.isDme()) { flags.insert("DME"); }
    if (vor.isTacan()) { flags.insert("TACAN"); }
    if (vor.hasGlideSlope()) { flags.insert("GS"); }
    if (vor.hasBackCourse()) { flags.insert("BC"); }
    
    return flags;
}


/**
 * Print VOR details.
 */
static void printVOR(const Facilities::VORData& vor)
{
    const auto flags = vorFlags(vor);
    std::string flagsStr{};
    for (const auto& flag : flags) {
        if (!flagsStr.empty()) {
            flagsStr += ",";
        }
        flagsStr += flag;
    }
    if (flagsStr.empty()) {
        flagsStr = "None";
    }

    std::cout << std::format("VOR '{}':\n", vor.name());
    std::cout << std::format("  Frequency: {:.3f} MHz\n", vor.frequencyMHz());
    switch (vor.type()) {
    case Facilities::VORType::VOR_Unknown:
        std::cout << "  Type: Unknown\n";
        break;
    case Facilities::VORType::VOR_Terminal:
        std::cout << "  Type: Terminal\n";
        break;
    case Facilities::VORType::VOR_LowAltitude:
        std::cout << "  Type: Low Altitude\n";
        break;
    case Facilities::VORType::VOR_HighAltitude:
        std::cout << "  Type: High Altitude\n";
        break;
    case Facilities::VORType::VOR_ILS:
        std::cout << "  Type: ILS\n";
        break;
    case Facilities::VORType::VOR_VOT:
        std::cout << "  Type: VOT\n";
        break;
    }
    std::cout << std::format("  Capabilities: {}\n", flagsStr);
    std::cout << std::format("  VOR Position: {:.6f}{}, {:.6f}{}, Alt {:f}ft\n", vor.vorLatitude(), degreeSymbol, vor.vorLongitude(), degreeSymbol, vor.vorAltitudeFeet());
    
    if (vor.isDme()) {
        if ((vor.isNav() && !vor.dmeAtNav()) || (vor.hasGlideSlope() && !vor.dmeAtGlideSlope())) {
            std::cout << std::format("  DME Position: {:.6f}{}, {:.6f}{}, Alt {:f}ft\n", vor.dmeLatitude(), degreeSymbol, vor.dmeLongitude(), degreeSymbol, vor.dmeAltitudeFeet());
        }
        if (vor.dmeAtNav()) {
            std::cout << "  DME co-located with NAV\n";
        }
        if (vor.dmeAtGlideSlope()) {
            std::cout << "  DME co-located with Glide Slope\n";
        }
        std::cout << std::format("  DME Bias: {:.2f} NM\n", vor.dmeBias());
    }
    
    if (vor.hasGlideSlope()) {
        std::cout << std::format("  Glide Slope: {:.2f}{}\n", vor.glideSlopeDegrees(), degreeSymbol);
        std::cout << std::format("  GS Position: {:.6f}{}, {:.6f}{}, Alt {:f}ft\n", vor.gsLatitude(), degreeSymbol, vor.gsLongitude(), degreeSymbol, vor.gsAltitudeFeet());
    }
    
    if (vor.isNav() && vor.localizerHeading() != 0) {
        std::cout << std::format("  Localizer: {:.2f}{}, Width {:.2f}{}\n",
            vor.localizerHeading(), degreeSymbol,
            vor.localizerWidth(), degreeSymbol);
        std::cout << std::format("  ILS Category: {}\n", static_cast<int>(vor.lsCategory()));
        if (vor.hasBackCourse()) {
            std::cout << "  Has Back Course\n";
        }
    }
    
    if (vor.isTacan()) {
        std::cout << std::format("  TACAN Position: {:.6f}{}, {:.6f}{}, Alt {:f}ft\n", vor.tacanLatitude(), degreeSymbol, vor.tacanLongitude(), degreeSymbol, vor.tacanAltitudeFeet());
    }
    
    std::cout << std::format("  Nav Range: {:.1f} NM\n", vor.navRange());
    std::cout << std::format("  Magnetic Variation: {:.2f}{}\n", vor.magVar(), degreeSymbol);
    std::cout << std::format("  Reference: {}\n", vor.isTrueReferenced() ? "True" : "Magnetic");
}


/**
 * Print a single frequency entry.
 * 
 * @param frequency The frequency to print.
 */
static void printFrequency(const Facilities::FrequencyData& frequency)
{
    std::string type{};
    switch (frequency.type()) {
    case Facilities::FrequencyType::None:
        break;
    case Facilities::FrequencyType::ATIS:
        type = "ATIS";
        break;
    case Facilities::FrequencyType::Multicom:
        type = "Multicom";
        break;
    case Facilities::FrequencyType::Unicom:
        type = "Unicom";
        break;
    case Facilities::FrequencyType::CTAF:
        type = "CTAF";
        break;
    case Facilities::FrequencyType::Ground:
        type = "Ground";
        break;
    case Facilities::FrequencyType::Tower:
        type = "Tower";
        break;
    case Facilities::FrequencyType::Clearance:
        type = "Clearance";
        break;
    case Facilities::FrequencyType::Approach:
        type = "Approach";
        break;
    case Facilities::FrequencyType::Departure:
        type = "Departure";
        break;
    case Facilities::FrequencyType::Center:
        type = "Center";
        break;
    case Facilities::FrequencyType::FSS:
        type = "FSS";
        break;
    case Facilities::FrequencyType::AWOS:
        type = "AWOS";
        break;
    case Facilities::FrequencyType::ASOS:
        type = "ASOS";
        break;
    case Facilities::FrequencyType::CPT:
        type = "CPT";
        break;
    case Facilities::FrequencyType::GCO:
        type = "GCO";
        break;
    }
    std::cout << std::format("- {:10}: {:.3f} MHz ('{}')\n", type, frequency.frequencyMHz(), frequency.name());
}


/**
 * Print all airport data including frequencies and taxi parkings.
 * 
 * @param airport The airport to print.
 */
static void printAirport(const Facilities::AirportFacility& airport)
{
    std::cout << std::format("Airport {} has {} frequencies and {} taxi parkings:\n",
        airport.data.icao(),
        airport.frequencies.size(),
        airport.taxiParkings.size());
    
    if (airport.haveFrequencies()) {
        std::cout << "\nFrequencies:\n";
        for (const auto& frequency : airport.frequencies) {
            printFrequency(frequency);
        }
    }
    
    if (airport.haveTaxiParkings()) {
        std::cout << "\nTaxi Parkings:\n";
        for (const auto& [parkingKey, parking] : airport.taxiParkings) {
            printTaxiParking(parking.data.formatParkingName(), parking, airport.data);
        }
    }
}


/**
 * List detailed information about a specific airport.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param icao The ICAO code of the airport to list.
 * @param region The region code of the airport to list.
 */
static void listAirportDetails(auto& connectionHandler, const std::string& icao, const std::string& region)
{
    bool listingDone{ false };

    // Build a definition of what we want to receive
    static constexpr std::size_t BuilderSize = 128;
    auto builder = Facilities::Builder<BuilderSize>()
        .airport()
            .allFields()
            .frequency()
                .allFields()
            .end()
            .taxiParking()
                .allFields()
            .end()
        .end();
    FacilityHandler<WindowsEventHandler<true, ConsoleLogger>> facilityHandler(connectionHandler);
    const FacilityDefinitionId defId = facilityHandler.buildDefinition(builder);
    Facilities::AirportFacility airport;

    auto request = facilityHandler.requestFacilityData(defId, icao, region,
        [&airport](const Messages::FacilityDataMsg& msg) {
            if (Facilities::AirportData::isAirportData(msg)) {
                airport.data = Facilities::AirportData::from(msg);
            }
            else if (Facilities::FrequencyData::isFrequencyData(msg)) {
                airport.frequencies.push_back(Facilities::FrequencyData::from(msg));
            }
            else if (Facilities::TaxiParkingData::isTaxiParkingData(msg)) {
                const auto& parkingData = Facilities::TaxiParkingData::from(msg);
                const Facilities::ParkingKey key{ parkingData.name(), parkingData.number(), parkingData.suffix() };
                airport.taxiParkings.emplace(key, Facilities::TaxiParkingFacility{ .data = parkingData });
            }
        },
        [&listingDone]() {
            std::cout << "Finished listing airport parkings.\n";
            listingDone = true;
        },
        [&listingDone](const Messages::FacilityMinimalListMsg& msg) {
            std::cerr << std::format("Received minimal facility list with {} items.\n", msg.dwArraySize);
            listingDone = true;
        }
    );
    static constexpr auto timeout = 30s;
    std::cout << std::format("Listing facilities, will timeout after {} seconds...\n", timeout.count());
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
    printAirport(airport);
}


/**
 * List detailed information about a specific VOR.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param ident The identifier of the VOR to list.
 * @param region The region code of the VOR to list.
 */
static void listVORDetails(auto& connectionHandler, const std::string& ident, const std::string& region)
{
    bool listingDone{ false };

    // Build a definition of what we want to receive
    static constexpr std::size_t BuilderSize = 128;
    auto builder = Facilities::Builder<BuilderSize>()
        .vor()
            .allFields()
        .end();
    FacilityHandler<WindowsEventHandler<true, ConsoleLogger>> facilityHandler(connectionHandler);
    const FacilityDefinitionId defId = facilityHandler.buildDefinition(builder);
    Facilities::VORData vor{};
    bool vorReceived{ false };

    auto request = facilityHandler.requestFacilityData(defId, ident, region,
        [&vor, &vorReceived](const Messages::FacilityDataMsg& msg) {
            if (Facilities::VORData::isVORData(msg)) {
                vor = Facilities::VORData::from(msg);
                vorReceived = true;
            }
        },
        [&listingDone]() {
            std::cout << "Finished listing VOR details.\n";
            listingDone = true;
        },
        [&listingDone](const Messages::FacilityMinimalListMsg& msg) {
            std::cerr << std::format("Received minimal facility list with {} items.\n", msg.dwArraySize);
            listingDone = true;
        }
    );
    static constexpr auto timeout = 30s;
    std::cout << std::format("Listing VOR, will timeout after {} seconds...\n", timeout.count());
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
    
    if (vorReceived) {
        printVOR(vor);
    } else {
        std::cerr << std::format("VOR '{}' not found.\n", ident);
    }
}


/**
 * List airports matching the given filters.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param icaoPattern Regex pattern to filter airports by ICAO/ident.
 * @param regionFilter Exact region code to filter airports by (optional).
 */
static void listAirports(ThisSimConnectHandler& connectionHandler, std::string_view icaoPattern, std::string_view regionFilter)
{
    FacilityListHandler<ThisSimConnectHandler> facilityListHandler(connectionHandler);

    std::regex icaoRegex;
    bool useRegex = false;
    
    if (!icaoPattern.empty()) {
        try {
            icaoRegex = std::regex(std::string(icaoPattern), std::regex_constants::icase);
            useRegex = true;
            std::cout << std::format("Listing airports matching pattern '{}'", icaoPattern);
        } catch (const std::regex_error& e) {
            std::cerr << std::format("Invalid regex pattern '{}': {}\n", icaoPattern, e.what());
            return;
        }
    } else {
        std::cout << "Listing all airports";
    }
    
    if (!regionFilter.empty()) {
        std::cout << std::format(" in region '{}'", regionFilter);
    }
    std::cout << ":\n";

    bool listingDone{ false };
    auto request = facilityListHandler.listAirports(FacilitiesListScope::allFacilities,
        [&icaoRegex, useRegex, regionFilter](std::string_view ident, std::string_view region, const AirportDetails& details) {
            // Filter by region if specified
            if (!regionFilter.empty() && region != regionFilter) {
                return;
            }
            // Filter by ICAO pattern if specified
            if (useRegex && !std::regex_match(std::string(ident), icaoRegex)) {
                return;
            }
            std::cout << std::format("- {:5}: {:8.3f}{}{}, {:7.3f}{}{}, Alt {:6}ft (Region {})\n",
                ident,
                details.latitudeNormalized(), degreeSymbol, details.latitudeDirection(),
                details.longitudeNormalized(), degreeSymbol, details.longitudeDirection(),
                details.altitudeFeet(),
                region);
        },
        [&listingDone]() { listingDone = true; }
    );

    static constexpr auto timeout = 30s;
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
}


/**
 * List VORs matching the given filters.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param identPattern Regex pattern to filter VORs by ident.
 * @param regionFilter Exact region code to filter VORs by (optional).
 */
static void listVORs(ThisSimConnectHandler& connectionHandler, std::string_view identPattern, std::string_view regionFilter)
{
    FacilityListHandler<ThisSimConnectHandler> facilityListHandler(connectionHandler);

    std::regex identRegex;
    bool useRegex = false;
    
    if (!identPattern.empty()) {
        try {
            identRegex = std::regex(std::string(identPattern), std::regex_constants::icase);
            useRegex = true;
            std::cout << std::format("Listing VORs matching pattern '{}'", identPattern);
        } catch (const std::regex_error& e) {
            std::cerr << std::format("Invalid regex pattern '{}': {}\n", identPattern, e.what());
            return;
        }
    } else {
        std::cout << "Listing all VORs";
    }
    
    if (!regionFilter.empty()) {
        std::cout << std::format(" in region '{}'", regionFilter);
    }
    std::cout << ":\n";

    bool listingDone{ false };
    auto request = facilityListHandler.listVORs(FacilitiesListScope::allFacilities,
        [&identRegex, useRegex, regionFilter]([[maybe_unused]] std::string_view ident, [[maybe_unused]] std::string_view region, [[maybe_unused]] const VorDetails& details) { // NOLINT(bugprone-easily-swappable-parameters)
            // Filter by region if specified
            if (!regionFilter.empty() && region != regionFilter) {
                return;
            }
            // Filter by ident pattern if specified
            if (useRegex && !std::regex_match(std::string(ident), identRegex)) {
                return;
            }

            std::cout << std::format("- {:5}: {:8.3f}{}{}, {:7.3f}{}{}, MagVar {:6.2f}{}{}, Alt {:6}ft, {:7.3f} MHz [{:3} {:3} {:3} {:3}] (Region {})\n",
                ident,
                details.latitudeNormalized(), degreeSymbol, details.latitudeDirection(),
                details.longitudeNormalized(), degreeSymbol, details.longitudeDirection(),
                details.MagVarNormalized(), degreeSymbol, details.magVarDirection(),
                details.altitudeFeet(),
                details.frequencyMHz(),
                details.hasNavSignal()  ? "NAV" : "",
                details.hasDME()        ? "DME" : "",
                details.hasLocalizer()  ? "LOC" : "",
                details.hasGlideSlope() ? "GS" : "",
                region);
        },
        [&listingDone]() { listingDone = true; }
    );
    static constexpr auto timeout = 30s;
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
}


/**
 * List NDBs matching the given filters.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param identPattern Regex pattern to filter NDBs by ident.
 * @param regionFilter Exact region code to filter NDBs by (optional).
 */
static void listNDBs(ThisSimConnectHandler& connectionHandler, std::string_view identPattern, std::string_view regionFilter)
{
    FacilityListHandler<ThisSimConnectHandler> facilityListHandler(connectionHandler);

    std::regex identRegex;
    bool useRegex = false;
    
    if (!identPattern.empty()) {
        try {
            identRegex = std::regex(std::string(identPattern), std::regex_constants::icase);
            useRegex = true;
            std::cout << std::format("Listing NDBs matching pattern '{}'", identPattern);
        } catch (const std::regex_error& e) {
            std::cerr << std::format("Invalid regex pattern '{}': {}\n", identPattern, e.what());
            return;
        }
    } else {
        std::cout << "Listing all NDBs";
    }
    
    if (!regionFilter.empty()) {
        std::cout << std::format(" in region '{}'", regionFilter);
    }
    std::cout << ":\n";

    bool listingDone{ false };
    auto request = facilityListHandler.listNDBs(FacilitiesListScope::allFacilities,
        [&identRegex, useRegex, regionFilter](std::string_view ident, std::string_view region, const NdbDetails& details) {
            // Filter by region if specified
            if (!regionFilter.empty() && region != regionFilter) {
                return;
            }
            // Filter by ident pattern if specified
            if (useRegex && !std::regex_match(std::string(ident), identRegex)) {
                return;
            }
            
            std::cout << std::format("- {:5}: {:8.3f}{}{}, {:7.3f}{}{}, MagVar {:6.2f}{}{}, Alt {:6}ft, {:6.2f} kHz (Region {})\n",
                ident,
                details.latitudeNormalized(), degreeSymbol, details.latitudeDirection(),
                details.longitudeNormalized(), degreeSymbol, details.longitudeDirection(),
                details.MagVarNormalized(), degreeSymbol, details.magVarDirection(),
                details.altitudeFeet(),
                details.frequencyKHz(),
                region);
        },
        [&listingDone]() { listingDone = true; }
    );
    static constexpr auto timeout = 30s;
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
}


/**
 * List Waypoints matching the given filters.
 * 
 * @param connectionHandler The connection handler to use for communication.
 * @param identPattern Regex pattern to filter waypoints by ident.
 * @param regionFilter Exact region code to filter waypoints by (optional).
 */
static void listWaypoints(ThisSimConnectHandler& connectionHandler, std::string_view identPattern, std::string_view regionFilter)
{
    FacilityListHandler<ThisSimConnectHandler> facilityListHandler(connectionHandler);

    std::regex identRegex;
    bool useRegex = false;
    
    if (!identPattern.empty()) {
        try {
            identRegex = std::regex(std::string(identPattern), std::regex_constants::icase);
            useRegex = true;
            std::cout << std::format("Listing waypoints matching pattern '{}'", identPattern);
        } catch (const std::regex_error& e) {
            std::cerr << std::format("Invalid regex pattern '{}': {}\n", identPattern, e.what());
            return;
        }
    } else {
        std::cout << "Listing all waypoints";
    }
    
    if (!regionFilter.empty()) {
        std::cout << std::format(" in region '{}'", regionFilter);
    }
    std::cout << ":\n";

    bool listingDone{ false };
    auto request = facilityListHandler.listWaypoints(FacilitiesListScope::allFacilities,
        [&identRegex, useRegex, regionFilter](std::string_view ident, std::string_view region, const WaypointDetails& details) {
            // Filter by region if specified
            if (!regionFilter.empty() && region != regionFilter) {
                return;
            }
            // Filter by ident pattern if specified
            if (useRegex && !std::regex_match(std::string(ident), identRegex)) {
                return;
            }
            
            std::cout << std::format("- {:5}: {:8.3f}{}{}, {:7.3f}{}{}, MagVar {:6.2f}{}{}, Alt {:6}ft (Region {})\n",
                ident,
                details.latitudeNormalized(), degreeSymbol, details.latitudeDirection(),
                details.longitudeNormalized(), degreeSymbol, details.longitudeDirection(),
                details.MagVarNormalized(), degreeSymbol, details.magVarDirection(),
                details.altitudeFeet(),
                region);
        },
        [&listingDone]() { listingDone = true; }
    );
    static constexpr auto timeout = 30s;
    connectionHandler.handleUntilOrTimeout([&listingDone]() { return listingDone; }, timeout);
    request.stop();
}


auto main(int argc, const char *argv[]) -> int// NOLINT(bugprone-exception-escape,readability-function-cognitive-complexity)
{

    static constexpr const char *appName = "List titles and liveries";

    auto args = gatherArgs(argc, argv);

    const std::string icao = args.contains("Arg1") ? args["Arg1"] : "";
    const std::string region = args.contains("region") ? args["region"] : "";
    const std::string type = args.contains("type") ? args["type"] : "airport";
    const std::string filter = args.contains("filter") ? args["filter"] : "";

    const LogLevel logLevel = args.contains("debug") ? LogLevel::Debug : LogLevel::Info;

    // Connect to the simulator
    WindowsEventConnection<true, ConsoleLogger> connection(appName);
    connection.logger().level(logLevel);
    WindowsEventHandler<true, ConsoleLogger> connectionHandler(connection);
    connectionHandler.logger().level(logLevel);

    // connectionHandler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    connectionHandler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    connectionHandler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (!connection.open()) {
        std::cerr << "[ABORTING: Failed to connect to the simulator]\n";
        return 1;
    }

    if (type == "airport") {
        if (!icao.empty()) {
            listAirportDetails(connectionHandler, icao, region);
        } else {
            listAirports(connectionHandler, filter, region);
        }
    } else if (type == "vor") {
        if (!icao.empty()) {
            listVORDetails(connectionHandler, icao, region);
        } else {
            listVORs(connectionHandler, filter, region);
        }
    } else if (type == "ndb") {
        listNDBs(connectionHandler, filter, region);
    } else if (type == "waypoint") {
        listWaypoints(connectionHandler, filter, region);
    } else {
        std::cerr << std::format("Unknown type '{}' specified. Supported types are 'airport', 'vor', 'ndb', and 'waypoint'.\n", type);
    }
    connection.close();

    return 0;
}