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


#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#include <iostream>
#include <format>
#include <string>
#include <algorithm>
#include <vector>
#include <regex>
#include <cmath>
#include <numbers>

#include "dataset.hpp"
#include "bag_index.hpp"
#include "airport.hpp"
#include "parking.hpp"
#include "livery.hpp"


/**
 * A shorthand test if we need to avoid using MSFS 2024 specific features.
 */
#if defined(SIMCONNECT_TYPEDEF)
#define MSFS_2024_SDK 1     // NOLINT(cppcoreguidelines-macro-usage)
#else
#define MSFS_2024_SDK 0     // NOLINT(cppcoreguidelines-macro-usage)
#endif


template <typename enum_type>
consteval DWORD dword(enum_type v) { return static_cast<DWORD>(v); }

inline const uint8_t *toBytePtr(const void *ptr) { return reinterpret_cast<const uint8_t *>(ptr); }
inline const int32_t *toIntPtr(const void *ptr) { return reinterpret_cast<const int32_t *>(ptr); }
inline const DWORD *toDwordPtr(const void *ptr) { return reinterpret_cast<const DWORD *>(ptr); }
inline const char *toCharPtr(const void *ptr) { return reinterpret_cast<const char *>(ptr); }
inline const float *toFloatPtr(const void *ptr) { return reinterpret_cast<const float *>(ptr); } 


template <typename Recv>
inline const Recv* toRecvPtr(const void *ptr) { return reinterpret_cast<const Recv *>(ptr); }

inline bool isTagged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_TAGGED) != 0); }
inline bool isChanged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_CHANGED) != 0); }

static HANDLE hSimConnect{ nullptr };		// The connection handle
static bool connected{ false };				// Do we have a live connection?

static std::map<std::string, std::string> args;


enum RequestIds : DWORD {
    REQUEST_AIRPORTS = 1,
    REQUEST_LIVERIES = 2,
    REQUEST_PARKINGS = 3
};
enum DataDefinitions : DWORD {
    DATADEF_PARKINGS = 1
};


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
static void handleException(const SIMCONNECT_RECV_EXCEPTION& msg)
{

    std::cout << std::format("Received an exception type {}:\n", msg.dwException);
    if (msg.dwSendID != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_SENDID)
    {
        std::cout << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
    }
    if (msg.dwIndex != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_INDEX)
    {
        std::cout << std::format("- Regarding parameter {}.\n", msg.dwIndex);
    }

    const SIMCONNECT_EXCEPTION exc{static_cast<SIMCONNECT_EXCEPTION>(msg.dwException)};
    switch (exc)
    {
    case SIMCONNECT_EXCEPTION_NONE: // Should never happen
        std::cerr << "No exception.\n";
        break;
    case SIMCONNECT_EXCEPTION_ERROR:
        std::cerr << "Some unspecific error has occurred.\n";
        break;
    case SIMCONNECT_EXCEPTION_SIZE_MISMATCH:
        std::cerr << "The size of the parameter does not match the expected size.\n";
        break;
    case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:
        std::cerr << "The parameter is not a recognized ID.\n";
        break;
    case SIMCONNECT_EXCEPTION_UNOPENED:
        std::cerr << "The connection has not been opened.\n";
        break;
    case SIMCONNECT_EXCEPTION_VERSION_MISMATCH:
        std::cerr << "This version of SimConnect cannot work with this version of the simulator.\n";
        break;
    case SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS:
        std::cerr << "The maximum number of (input/notification) groups has been reached. (currently 20)\n";
        break;
    case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:
        std::cerr << "The parameter is not a recognized name.\n";
        break;
    case SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES:
        std::cerr << "The maximum number of event names has been reached. (currently 1000)\n";
        break;
    case SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE:
        std::cerr << "The event ID is already in use.\n";
        break;
    case SIMCONNECT_EXCEPTION_TOO_MANY_MAPS:
        std::cerr << "The maximum number of mapings has been reached. (currently 20)\n";
        break;
    case SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS:
        std::cerr << "The maximum number of objects has been reached. (currently 1000)\n";
        break;
    case SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS:
        std::cerr << "The maximum number of requests has been reached. (currently 1000)\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT: // Legacy
        std::cerr << "The weather port is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR: // Legacy
        std::cerr << "The METAR string is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION: // Legacy
        std::cerr << "Unable to get the observation.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION: // Legacy
        std::cerr << "Unable to create the station.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION: // Legacy
        std::cerr << "Unable to remove the station.\n";
        break;
    case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:
        std::cerr << "The requested data cannot be converted to the specified data type.\n";
        break;
    case SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE:
        std::cerr << "The requested data cannot be transferred in the specified data size.\n";
        break;
    case SIMCONNECT_EXCEPTION_DATA_ERROR:
        std::cerr << "The data passed is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_INVALID_ARRAY:
        std::cerr << "The array passed to SetDataOnSimObject is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED:
        std::cerr << "The AI object could not be created.\n";
        break;
    case SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED:
        std::cerr << "The flight plan could not be loaded. Either it could not be found, or it contained an error.\n";
        break;
    case SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE:
        std::cerr << "The operation is not valid for the object type.\n";
        break;
    case SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION:
        std::cerr << "The operation is illegal. (AI or Weather)\n";
        break;
    case SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED:
        std::cerr << "The client is already subscribed to this event.\n";
        break;
    case SIMCONNECT_EXCEPTION_INVALID_ENUM:
        std::cerr << "The type enum value is unknown. (Probably an unknown type in RequestDataOnSimObjectType)\n";
        break;
    case SIMCONNECT_EXCEPTION_DEFINITION_ERROR:
        std::cerr << "The definition is invalid. (Probably a variable length requested in RequestDataOnSimObject)\n";
        break;
    case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
        std::cerr << "The ID is already in use. (Menu, DataDefinition item ID, ClientData mapping, or event to notification group)\n";
        break;
    case SIMCONNECT_EXCEPTION_DATUM_ID:
        std::cerr << "Unknown datum ID specified for SetDataOnSimObject.\n";
        break;
    case SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS:
        std::cerr << "The requested value is out of bounds. (radius of a RequestDataOnSimObjectType, or CreateClientData)\n";
        break;
    case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
        std::cerr << "A ClientData area with that name has already been created.\n";
        break;
    case SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE:
        std::cerr << "The AI object is outside the reality bubble.\n";
        break;
    case SIMCONNECT_EXCEPTION_OBJECT_CONTAINER:
        std::cerr << "The AI object creation failed. (container issue)\n";
        break;
    case SIMCONNECT_EXCEPTION_OBJECT_AI:
        std::cerr << "The AI object creation failed. (AI issue)\n";
        break;
    case SIMCONNECT_EXCEPTION_OBJECT_ATC:
        std::cerr << "The AI object creation failed. (ATC issue)\n";
        break;
    case SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE:
        std::cerr << "The AI object creation failed. (scheduling issue)\n";
        break;
    case SIMCONNECT_EXCEPTION_JETWAY_DATA:
        std::cerr << "Requesting JetWay data failed.\n";
        break;
    case SIMCONNECT_EXCEPTION_ACTION_NOT_FOUND:
        std::cerr << "The action was not found.\n";
        break;
    case SIMCONNECT_EXCEPTION_NOT_AN_ACTION:
        std::cerr << "The action was not a valid action.\n";
        break;
    case SIMCONNECT_EXCEPTION_INCORRECT_ACTION_PARAMS:
        std::cerr << "The action parameters were incorrect.\n";
        break;
    case SIMCONNECT_EXCEPTION_GET_INPUT_EVENT_FAILED:
        std::cerr << "The input event name was not found. (GetInputEvent)\n";
        break;
    case SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED:
        std::cerr << "The input event name was not found. (SetInputEvent)\n";
        break;
#if MSFS_2024_SDK
    case SIMCONNECT_EXCEPTION_INTERNAL:
        std::cerr << "An internal SimConnect error has occurred.\n";
        break;
#endif
        // No default; we want an error if we miss one
    }
}


static DataSet<Airport> airports;
static BagIndex<std::string> airportsByRegion; // Maps region codes to sets of airports

static DataSet<Parking> parkings;
static BagIndex<std::string> parkingsByAirport; // Maps airport ICAO codes to sets of parkings

static DataSet<Livery> liveries;
static BagIndex<std::string> liveriesByTitle; // Maps aircraft titles to sets of liveries
static BagIndex<std::string> liveriesByLivery; // Maps livery names to sets of aircraft titles


constexpr static const char* parkingTypes[] = {
    "",
    "Ramp GA",
    "Ramp GA Small",
	"Ramp GA Medium",
    "Ramp GA Large",
    "Ramp Cargo",
    "Ramp Mil Cargo",
	"Ramp Mil Combat",
	"Gate Small",
    "Gate Medium",
    "Gate Heavy",
    "Dock GA",
    "Fuel",
	"Vehicle",
    "Ramp GA Extra",
	"Gate Extra",
};
constexpr static const char* taxiPointTypes[] = {
    "",
    "Normal",
    "Hold Short",
    "",
    "ILS Hold Short",
    "Hold Short No Draw",
    "ILS Hold Short No Draw",
};
constexpr static const char* parkingNames[] = {
    "",
    "Parking",
    "N Parking",
    "NE Parking",
	"E Parking",
    "SE Parking",
    "S Parking",
    "SW Parking",
    "W Parking",
    "NW Parking",
    "Gate",
    "Dock",
    "Gate A",
	"Gate B",
    "Gate C",
    "Gate D",
    "Gate E",
    "Gate F",
    "Gate G",
    "Gate H",
    "Gate I",
    "Gate J",
    "Gate K",
    "Gate L",
	"Gate M",
    "Gate N",
    "Gate O",
    "Gate P",
    "Gate Q",
    "Gate R",
    "Gate S",
    "Gate T",
    "Gate U",
    "Gate V",
    "Gate W",
    "Gate X",
    "Gate Y",
	"Gate Z",
};
constexpr static const char* parkingOrientation[] = {
    "Forward",
    "Reverse",
};
constexpr static const char* facilityDataType[] = {
    "Airport",              //SIMCONNECT_FACILITY_DATA_AIRPORT,
    "Runway",               //SIMCONNECT_FACILITY_DATA_RUNWAY,
    "Start",                //SIMCONNECT_FACILITY_DATA_START,
    "Frequency",            //SIMCONNECT_FACILITY_DATA_FREQUENCY,
    "Helipad",              //SIMCONNECT_FACILITY_DATA_HELIPAD,
    "Approach",             //SIMCONNECT_FACILITY_DATA_APPROACH,
    "Approach transition",  //SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION,
    "Approach leg",         //SIMCONNECT_FACILITY_DATA_APPROACH_LEG,
    "Final approach leg",   //SIMCONNECT_FACILITY_DATA_FINAL_APPROACH_LEG,
    "Missed approach leg",  //SIMCONNECT_FACILITY_DATA_MISSED_APPROACH_LEG,
    "Departure",            //SIMCONNECT_FACILITY_DATA_DEPARTURE,
    "Arrival",              //SIMCONNECT_FACILITY_DATA_ARRIVAL,
    "Runway transition",    //SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION,
    "Enroute transition",   //SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION,
    "Taxi point",           //SIMCONNECT_FACILITY_DATA_TAXI_POINT,
    "Taxi parking",         //SIMCONNECT_FACILITY_DATA_TAXI_PARKING,
    "Taxi path",            //SIMCONNECT_FACILITY_DATA_TAXI_PATH,
    "Taxi name",            //SIMCONNECT_FACILITY_DATA_TAXI_NAME,
    "Jetway",               //SIMCONNECT_FACILITY_DATA_JETWAY,
    "VOR",                  //SIMCONNECT_FACILITY_DATA_VOR,
    "NDB",                  //SIMCONNECT_FACILITY_DATA_NDB,
    "Waypoint",             //SIMCONNECT_FACILITY_DATA_WAYPOINT,
    "Route",                //SIMCONNECT_FACILITY_DATA_ROUTE,
    "Pavement",             //SIMCONNECT_FACILITY_DATA_PAVEMENT,
    "Lights",               //SIMCONNECT_FACILITY_DATA_APPROACH_LIGHTS,
    "vasi",                 //SIMCONNECT_FACILITY_DATA_VASI,
    "vdgs",                 //SIMCONNECT_FACILITY_DATA_VDGS,
    "Holding pattern",      //SIMCONNECT_FACILITY_DATA_HOLDING_PATTERN,
    "Parking airline",      //SIMCONNECT_FACILITY_DATA_TAXI_PARKING_AIRLINE,
};

#pragma pack(push, 1) // Ensure no padding bytes are added to the structure
struct AirportData {
    char name[32];
    char longName[64];
	char icao[8];
    char region[8];
    double latitude;
    double longitude;
    double altitude;
};
struct ParkingData {
    int32_t type;
    int32_t taxi_point_type;
    int32_t name;
    int32_t suffix;
    uint32_t number;
    int32_t orientation;
    float heading;
    float radius;
    float bias_x;
	float bias_z;
    int32_t n_airlines;
};
#pragma pack(pop) // Restore previous packing alignment

/**
 * Handle messages from SimConnect.
 *
 * @param hEvent The event handle to wait for.
 */
static void handle_messages(HANDLE hEvent)
{
	while (connected && (::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)) {
		SIMCONNECT_RECV* pData{ nullptr };
		DWORD cbData{ 0 };
		HRESULT hr{ S_OK };

        static Airport currentAirport;

		while (SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
			switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
                break;

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const SIMCONNECT_RECV_OPEN *pOpen = toRecvPtr<SIMCONNECT_RECV_OPEN>(pData);


                std::cerr << std::format("[Connected to '{}' version {}.{} (build {}.{}) using SimConnect version {}.{} (build {}.{})]\n",
                                          pOpen->szApplicationName,
                                          pOpen->dwApplicationVersionMajor,
                                          pOpen->dwApplicationVersionMinor,
                                          pOpen->dwApplicationBuildMajor,
                                          pOpen->dwApplicationBuildMinor,
                                          pOpen->dwSimConnectVersionMajor,
                                          pOpen->dwSimConnectVersionMinor,
                                          pOpen->dwSimConnectBuildMajor,
                                          pOpen->dwSimConnectBuildMinor);
            }
            break;

            case SIMCONNECT_RECV_ID_QUIT:
            {
                std::cerr << "Simulator is shutting down.\n";
                connected = false;
            }
            break;

            case SIMCONNECT_RECV_ID_AIRPORT_LIST:
            {
                auto pAirportList = toRecvPtr<SIMCONNECT_RECV_AIRPORT_LIST>(pData);
                
                // Add each airport to the Airport class collection
                for (DWORD i = 0; i < pAirportList->dwArraySize; ++i) {
                    const auto& airportData = pAirportList->rgData[i];
                    airports.add({airportData.Ident, airportData.Region, airportData.Latitude, airportData.Longitude, airportData.Altitude});
					airportsByRegion.add(airportData.Region, airportData.Ident);
                }
                
                // Print progress
                std::cerr << std::format("[Received {} airports (part {} of {}), total collected: {}]\n", 
                                          pAirportList->dwArraySize, 
                                          pAirportList->dwEntryNumber + 1, 
                                          pAirportList->dwOutOf,
                                          airports.size());
                
                // If this was the last part, sort and print all airports
                if (pAirportList->dwEntryNumber == pAirportList->dwOutOf - 1) {
                    connected = false; // Exit the message loop
                }
            }
            break;

            case SIMCONNECT_RECV_ID_FACILITY_DATA:
            {
				const auto* pFacilityData = toRecvPtr<SIMCONNECT_RECV_FACILITY_DATA>(pData);
     //           std::cerr << std::format("[Received facility data for request ID {} (unique request id {}, parent id {})]\n", 
     //                                     pFacilityData->UserRequestId, pFacilityData->UniqueRequestId, pFacilityData->ParentUniqueRequestId);
     //           std::cerr << std::format("  Type: {}, IsListItem: {}, ItemIndex: {}, ListSize: {}\n", 
					//facilityDataType[pFacilityData->Type], pFacilityData->IsListItem, pFacilityData->ItemIndex, pFacilityData->ListSize);

                if (pFacilityData->Type == SIMCONNECT_FACILITY_DATA_AIRPORT) {
                    const auto* pAirport = reinterpret_cast<const AirportData*>(&(pFacilityData->Data));
                    currentAirport = { pAirport->name, pAirport->longName, pAirport->icao, pAirport->region, pAirport->latitude, pAirport->longitude, pAirport->altitude };
                    std::cerr << "  Airport:\n    Name: " << currentAirport.getName() << "\n";
                    if (!currentAirport.getLongName().empty() && (currentAirport.getLongName() != currentAirport.getName())) {
                        std::cerr << "    Long Name: " << currentAirport.getLongName() << "\n";
					}
                    else {
                        std::cerr << "          No long name.\n";
                    }
                    std::cerr << "    ICAO: " << currentAirport.getIcao() << "\n";
                    if (!currentAirport.getRegion().empty()) {
                        std::cerr << "    Region: " << currentAirport.getRegion() << "\n";
                    }
					std::cerr << std::format("    Location: {:.6f}N, {:.6f}E at {:.1f} m\n", currentAirport.getLatitude(), currentAirport.getLongitude(), currentAirport.getAltitude());
                }
                if (pFacilityData->Type == SIMCONNECT_FACILITY_DATA_TAXI_PARKING) {
                    const auto* pFacility = reinterpret_cast<const ParkingData*>(&(pFacilityData->Data));
                    Parking parking{
						currentAirport.getIcao(),
                        pFacility->number,
                        parkingTypes[pFacility->type],
                        parkingNames[pFacility->name],
                        parkingNames[pFacility->suffix],
						taxiPointTypes[pFacility->taxi_point_type],
                        pFacility->orientation != 0,
                        pFacility->heading,
						pFacility->radius,
                        pFacility->bias_x,
                        pFacility->bias_z,
                        pFacility->n_airlines
                    };
                    
                    // Convert meter offsets to degree offsets
                    // 1 degree latitude ? 111,111 meters
                    // 1 degree longitude ? 111,111 * cos(latitude) meters
                    constexpr double METERS_PER_DEGREE_LAT = 111111.0;
                    const double METERS_PER_DEGREE_LON = 111111.0 * std::cos(currentAirport.getLatitude() * std::numbers::pi / 180.0);
                    
                    const double parkingLatitude = currentAirport.getLatitude() + (pFacility->bias_z / METERS_PER_DEGREE_LAT);
                    const double parkingLongitude = currentAirport.getLongitude() + (pFacility->bias_x / METERS_PER_DEGREE_LON);
                    
					std::cerr << "  Parking: " << parking.field("AirportICAO") << " " << parking.field("Name") << "\n"
                        << std::format("    Location: {:.6f}N, {:.6f}E at {:.1f} m\n",
                            parkingLatitude,
                            parkingLongitude, 
                            currentAirport.getAltitude());
                }

			}
			break;

            case SIMCONNECT_RECV_ID_FACILITY_DATA_END:
            {
                std::cerr << "[Last facility data received]\n";
				connected = false; // Exit the message loop
            }
			break;

            case SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST:
            {
                auto pLiveryList = toRecvPtr<SIMCONNECT_RECV_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST>(pData);
                
                // Add each livery to the Livery class collection
                for (DWORD i = 0; i < pLiveryList->dwArraySize; ++i) {
                    const auto& liveryData = pLiveryList->rgData[i];
                    liveries.add({liveryData.AircraftTitle, liveryData.LiveryName});
					liveriesByTitle.add(liveryData.AircraftTitle, liveryData.LiveryName);
					liveriesByLivery.add(liveryData.LiveryName, liveryData.AircraftTitle);
                }
                
                // Print progress
                std::cerr << std::format("[Received {} liveries (part {} of {}), total collected: {}]\n", 
                                          pLiveryList->dwArraySize, 
                                          pLiveryList->dwEntryNumber + 1, 
                                          pLiveryList->dwOutOf,
                                          liveries.size());
                
                // If this was the last part, exit the message loop
                if (pLiveryList->dwEntryNumber == pLiveryList->dwOutOf - 1) {
                    connected = false; // Exit the message loop
                }
            }
            break;

            default:
                std::cerr << std::format("[Ignoring message of type {} (length {} bytes)]\n", pData->dwID, pData->dwSize);
                break;
            }
        }
		if (connected) {
            Sleep(100);
        }
    }
}


static void print_airports(OutputFormat format) {
    if (airports.size() == 0) {
        std::cerr << "No airports to display.\n";
        return;
    }
    if (args.contains("icao")) {
        const std::string& icao = args["icao"];
        if (airports.contains(icao)) {
			airports.stream(std::cout, icao, format);
        } else {
            std::cerr << std::format("No airport found with ICAO code '{}'.\n", icao);
        }
	}
    else if (args.contains("region")) {
        std::string region = args["region"];
        if (airportsByRegion.contains(region)) {
            for (const auto& icao : airportsByRegion.get(region)) {
                airports.stream(std::cout, icao, format);
            }
        }
    }
    else {
		airports.streamAll(std::cout, format);
    }
}

static void print_liveries(OutputFormat format) {
    if (liveries.size() == 0) {
        std::cerr << "No liveries to display.\n";
        return;
    }
    if (args.contains("title")) {
        const std::string& title = args["title"];

        if (liveriesByTitle.contains(title)) {
            for (const auto& livery : liveriesByTitle.get(title)) {
                liveries.stream(std::cout, livery, format);
            }
        }
    }
    else if (args.contains("title-regexp")) {
        const std::string& titleRegexp = args["title-regexp"];
        std::cerr << std::format("Searching for liveries matching: '{}'\n", titleRegexp);
        std::regex re(titleRegexp);
        for (const auto& [title, livery] : liveries.all()) {
            if (std::regex_search(title, re)) {
                liveries.stream(std::cout, title, format);
            }
        }
    }
    else if (args.contains("livery")) {
        const std::string& liveryName = args["livery"];
        if (liveriesByLivery.contains(liveryName)) {
            for (const auto& title : liveriesByLivery.get(liveryName)) {
                liveries.stream(std::cout, title, format);
            }
        }
	}
    else if (args.contains("livery-regexp")) {
        const std::string& liveryRegexp = args["livery-regexp"];
		std::cerr << std::format("Searching for liveries matching: '{}'\n", liveryRegexp);
        std::regex re(liveryRegexp);
        for (const auto& [title, livery] : liveries.all()) {
            if (std::regex_search(livery.getLivery(), re)) {
                liveries.stream(std::cout, title, format);
            }
        }
	}
    else {
		liveries.streamAll(std::cout, format);
    }
}


static void addToFacilityDef(DWORD defId, const char* name) {
    HRESULT hr;
    hr = SimConnect_AddToFacilityDefinition(hSimConnect, defId, name);
    if (FAILED(hr)) {
        std::cerr << std::format("Failed to add '{}' to facility definition {}!\n", name, defId);
    }
	DWORD lastSendID;
    hr = SimConnect_GetLastSentPacketID(hSimConnect, &lastSendID);
    if (FAILED(hr)) {
        std::cerr << "Failed to get last SendID!\n";
        lastSendID = 0;
	}
	std::cerr << std::format("Added '{}' to facility definition {} with SendID {}.\n", name, defId, lastSendID);
}


/**
 * Run a list command.
 * 
 * @return Zero if ok, non-zero if not.
 */
static int runQuery(std::string name, OutputFormat format)
{
	HANDLE hEventHandle{ ::CreateEvent(NULL, FALSE, FALSE, NULL) };
	if (hEventHandle == NULL) {
        std::cerr << "Failed to create a Windows Event!\n";
        return 1;
    }

    HRESULT hr = SimConnect_Open(&hSimConnect, "msfs-list", nullptr, 0, hEventHandle, 0);

	if (SUCCEEDED(hr)) {
        std::cerr << "[Successfully connected to MSFS.]\n";
        bool abort_requested{ false };

        if (name.empty()) {
            std::cerr << "No list name specified.\n";
            std::cerr << "Available lists: airports, liveries\n";  // Update this line
            abort_requested = true;
		}
        else if (name == "airports") {
            // Clear any previous airport data
            airports.clear();
            
            // Request airport list
            HRESULT hrRequest = SimConnect_RequestFacilitiesList(hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, REQUEST_AIRPORTS);
            if (FAILED(hrRequest)) {
                std::cerr << "Failed to request airport list!\n";
                abort_requested = true;
            } else {
                std::cerr << "[Requesting airport list...]\n";
            }
        }
        else if (name == "liveries") {  // Add this entire block
            // Clear any previous livery data
            liveries.clear();
            
            // Request livery list for all aircraft types
            HRESULT hrRequest = SimConnect_EnumerateSimObjectsAndLiveries(hSimConnect, REQUEST_LIVERIES, SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
            if (FAILED(hrRequest)) {
                std::cerr << "Failed to request livery list!\n";
                abort_requested = true;
            } else {
                std::cerr << "[Requesting livery list...]\n";
            }
        }
        else if (name == "parkings") {
            if (!args.contains("icao")) {
                std::cerr << "Airport ICAO is required for listing parkings.\n";
				abort_requested = true;
            }
			std::string region{ "" };
            if (args.contains("region")) {
                region = args["region"];
			}

            addToFacilityDef(DATADEF_PARKINGS, "OPEN AIRPORT");
            addToFacilityDef(DATADEF_PARKINGS, "NAME");
            addToFacilityDef(DATADEF_PARKINGS, "NAME64");
            addToFacilityDef(DATADEF_PARKINGS, "ICAO");
            addToFacilityDef(DATADEF_PARKINGS, "REGION");
            addToFacilityDef(DATADEF_PARKINGS, "LATITUDE");
			addToFacilityDef(DATADEF_PARKINGS, "LONGITUDE");
			addToFacilityDef(DATADEF_PARKINGS, "ALTITUDE");

            addToFacilityDef(DATADEF_PARKINGS, "OPEN TAXI_PARKING");
            addToFacilityDef(DATADEF_PARKINGS, "TYPE");
            addToFacilityDef(DATADEF_PARKINGS, "TAXI_POINT_TYPE");
            addToFacilityDef(DATADEF_PARKINGS, "NAME");
            addToFacilityDef(DATADEF_PARKINGS, "SUFFIX");
            addToFacilityDef(DATADEF_PARKINGS, "NUMBER");
            addToFacilityDef(DATADEF_PARKINGS, "ORIENTATION");
            addToFacilityDef(DATADEF_PARKINGS, "HEADING");
            addToFacilityDef(DATADEF_PARKINGS, "RADIUS");
            addToFacilityDef(DATADEF_PARKINGS, "BIAS_X");
            addToFacilityDef(DATADEF_PARKINGS, "BIAS_Z");
            addToFacilityDef(DATADEF_PARKINGS, "N_AIRLINES");
            addToFacilityDef(DATADEF_PARKINGS, "CLOSE TAXI_PARKING");

            addToFacilityDef(DATADEF_PARKINGS, "CLOSE AIRPORT");

			HRESULT hrRequest = SimConnect_RequestFacilityData(hSimConnect, DATADEF_PARKINGS, REQUEST_PARKINGS, args["icao"].c_str(), region.c_str());
            if (FAILED(hrRequest)) {
                std::cerr << std::format("Failed to request parking data for airport '{}'!\n", args["icao"]);
                abort_requested = true;
            } else {
                std::cerr << std::format("[Requested parking data for airport '{}'...]\n", args["icao"]);
			}
        }
        else {
            std::cerr << std::format("Unknown list name '{}'\n", name);
            std::cerr << "Available lists: airports, liveries\n";  // Update this line
            abort_requested = true;
        }
        connected = true;
        if (!abort_requested) {
            handle_messages(hEventHandle);
            if (name == "airports") {
                print_airports(format);
            } else if (name == "liveries") {  // Add this
                print_liveries(format);
            }
        }
        hr = SimConnect_Close(hSimConnect);

        std::cerr << "[Disconnected from MSFS.]\n";
    }
	else {
        std::cerr << "Failed to connect to MSFS!\n";
    }
    return FAILED(hr); // Zero is "ok"
}


auto main(int argc, const char* argv[]) -> int
{
    OutputFormat format = OutputFormat::Text;
    std::string listName;

    args.clear();
	int fixedArg{ 0 };

	args["Arg" + std::to_string(fixedArg++)] = argv[0];

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.starts_with("--")) {
			auto eq = arg.find('=');
            if (eq != std::string::npos) {
                std::string key = arg.substr(2, eq - 2);
                std::string value = arg.substr(eq + 1);
                args[key] = value;
            } else {
                args[arg.substr(2)] = ""; // No value provided
            }
        }
        else if ((fixedArg == 1) && (listName == "")) {
            listName = arg;
        }
        else {
			args["Arg" + std::to_string(fixedArg++)] = arg;
        }
    }
    if (listName.empty()) {
        std::cerr << std::format("Usage: {} <list-name> [--format=text|csv|json|yaml]\n", argv[0]);
        return -1;
    }
    if (args.contains("format")) {
        const std::string& formatStr = args["format"];
        if (formatStr == "text") {
            format = OutputFormat::Text;
        } else if (formatStr == "csv") {
            format = OutputFormat::Csv;
        } else if (formatStr == "json") {
            format = OutputFormat::Json;
        } else if (formatStr == "yaml") {
            format = OutputFormat::Yaml;
        } else {
            std::cerr << std::format("Unknown format '{}'; using text format.\n", formatStr);
        }
	}
    return runQuery(listName, format);
}