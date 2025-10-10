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
#include <map>

#include <cmath>
#include <numbers>


constexpr static const char* appName = "List parkings";
static HANDLE hSimConnect{ nullptr };		// The connection handle
static HANDLE hEvent{ nullptr };			// The event handle (for efficient waiting for SimConnect messages)

static std::map<std::string, std::string> args;


constexpr static const SIMCONNECT_DATA_REQUEST_ID REQUEST_ID{ 1 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFINITION_ID{ 1 };


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
static void handleException(const SIMCONNECT_RECV_EXCEPTION& msg)
{

    printf("Received an exception type %u:\n", msg.dwException);
    if (msg.dwSendID != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_SENDID)
    {
        printf("- Related to a message with SendID %u.\n", msg.dwSendID);
    }
    if (msg.dwIndex != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_INDEX)
    {
        printf("- Regarding parameter %u.\n", msg.dwIndex);
    }

    const SIMCONNECT_EXCEPTION exc{ static_cast<SIMCONNECT_EXCEPTION>(msg.dwException) };
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
#if defined(SIMCONNECT_EXCEPTION_JETWAY_DATA)
    case SIMCONNECT_EXCEPTION_JETWAY_DATA:
        std::cerr << "Requesting JetWay data failed.\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_ACTION_NOT_FOUND)
    case SIMCONNECT_EXCEPTION_ACTION_NOT_FOUND:
        std::cerr << "The action was not found.\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_NOT_AN_ACTION)
    case SIMCONNECT_EXCEPTION_NOT_AN_ACTION:
        std::cerr << "The action was not a valid action.\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_INCORRECT_ACTION_PARAMS)
    case SIMCONNECT_EXCEPTION_INCORRECT_ACTION_PARAMS:
        std::cerr << "The action parameters were incorrect.\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_GET_INPUT_EVENT_FAILED)
    case SIMCONNECT_EXCEPTION_GET_INPUT_EVENT_FAILED:
        std::cerr << "The input event name was not found. (GetInputEvent)\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED)
    case SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED:
        std::cerr << "The input event name was not found. (SetInputEvent)\n";
        break;
#endif
#if defined(SIMCONNECT_EXCEPTION_INTERNAL)
    case SIMCONNECT_EXCEPTION_INTERNAL:
        break;
#endif
        // No default; we want an error if we miss one
    }
}


/**
 * Connect to the simulator. This will also create a Windows Event for message handling.
 *
 * @return true if the connection was successful, false otherwise.
 */
static bool connect()
{
    if (hEvent == nullptr)
    {
        hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (hEvent == nullptr) {
            std::cerr << std::format("Failed to create event: 0x{:08X}.\n", GetLastError());
            return false;
        }
    }

    HRESULT hr = SimConnect_Open(&hSimConnect, appName, nullptr, 0, hEvent, 0);
    if (FAILED(hr))
    {
        std::cerr << std::format("Failed to connect to SimConnect: 0x{:08X}\n", hr);
        return false;
    }

    return true;
}


/**
 * Disconnect from the simulator and close the Windows Event.
 */
static void disconnect()
{
    if (hSimConnect != nullptr) {
        std::cerr << "[Disconnecting from the simulator.]\n";
        SimConnect_Close(hSimConnect);
        hSimConnect = nullptr;
    }
    if (hEvent != nullptr) {
        std::cerr << "[Closing event handle.]\n";
        CloseHandle(hEvent);
        hEvent = nullptr;
    }
}


/**
 * Helper to convert a SIMCONNECT_RECV pointer to a more specific type.
 *
 * @tparam Recv The specific SIMCONNECT_RECV type to convert to.
 * @param ptr The pointer to convert.
 * @return The converted pointer.
 */
template <typename Recv>
inline const Recv* toRecvPtr(const void* ptr) { return reinterpret_cast<const Recv*>(ptr); }


#pragma pack(push, 1) // Ensure no padding bytes are added to the structure
struct AirportData {
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
    float bias_x;
    float bias_z;
};
#pragma pack(pop) // Restore previous packing alignment


static AirportData lastAirport{};


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



/**
 * Handle messages from SimConnect.
 * 
 * @param icao The ICAO code of the airport to query.
 * @param region The region code of the airport to query (optional).
 * @param parking The parking spot to filter for (optional).
 */
static void handle_messages(std::string icao, std::string region, std::string parking)
{
    bool connected{ true };
    while (connected && (::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)) {
        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };
        HRESULT hr{ S_OK };

        while (SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
            {
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
            }
                break;

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const SIMCONNECT_RECV_OPEN* pOpen = toRecvPtr<SIMCONNECT_RECV_OPEN>(pData);


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

            case SIMCONNECT_RECV_ID_FACILITY_DATA:
            {
                const auto* pFacilityData = toRecvPtr<SIMCONNECT_RECV_FACILITY_DATA>(pData);

                if (pFacilityData->Type == SIMCONNECT_FACILITY_DATA_AIRPORT) {
                    const auto* pAirport = reinterpret_cast<const AirportData*>(&(pFacilityData->Data));
                    if (icao != pAirport->icao) {
						std::cerr << std::format("Received data for unexpected airport '{}', expected '{}'.\n", pAirport->icao, icao);
						return; // Not the airport we requested
					}

                    if (!region.empty() && region != pAirport->region) {
                        std::cerr << std::format("Received data for unexpected airport '{}' in region '{}', expected region '{}'.\n", pAirport->icao, pAirport->region, region);
                        return; // Not the region we requested
					}
                    lastAirport = *pAirport;
                }
                else if (pFacilityData->Type == SIMCONNECT_FACILITY_DATA_TAXI_PARKING) {
                    const auto* pParking = reinterpret_cast<const ParkingData*>(&(pFacilityData->Data));
                    std::string parkingName{ "" };
                    if (pParking->name > 0 && pParking->name < static_cast<int32_t>(std::size(parkingNames))) {
                        parkingName = parkingNames[pParking->name];
                    }
                    if (pParking->number > 0) {
                        if (!parkingName.empty()) {
                            parkingName += " ";
                        }
                        parkingName += std::to_string(pParking->number);
					}
                    if (pParking->suffix > 0) {
                        if (!parkingName.empty()) {
                            parkingName += " ";
                        }
                        parkingName += static_cast<char>('A' + (pParking->suffix - 1));
					}

                    if (!parking.empty() && parking != parkingName) {
                        continue; // Not the parking spot we are looking for
                    }
                    // Convert meter offsets to degree offsets
                    // 1 degree latitude ? 111,111 meters
                    // 1 degree longitude ? 111,111 * cos(latitude) meters
                    constexpr double METERS_PER_DEGREE_LAT = 111111.0;
                    const double METERS_PER_DEGREE_LON = 111111.0 * std::cos(lastAirport.latitude * std::numbers::pi / 180.0);

                    const double parkingLatitude = lastAirport.latitude + (pParking->bias_z / METERS_PER_DEGREE_LAT);
                    const double parkingLongitude = lastAirport.longitude + (pParking->bias_x / METERS_PER_DEGREE_LON);

                    std::cout << std::format("Parking '{}': (Orientation {}, Heading {:03}) at Airport {} (Region {}) Lat {:.6f} Lon {:.6f} Alt {:.2f}m\n",
                        parkingName,
                        pParking->orientation ? "Reverse" : "Forward",
                        static_cast<int>(pParking->heading),
                        lastAirport.icao,
                        lastAirport.region,
                        parkingLatitude,
                        parkingLongitude,
                        lastAirport.altitude);
                }
                else {
                    std::cerr << std::format("Received unexpected facility data type {}.\n", pFacilityData->Type);
				}
            }
            break;

            case SIMCONNECT_RECV_ID_FACILITY_DATA_END:
            {
                std::cerr << "[Last facility data received]\n";
                connected = false; // Exit the message loop
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


static void addToFacilityDef(DWORD defId, const char* name) {
    HRESULT hr;
    hr = SimConnect_AddToFacilityDefinition(hSimConnect, defId, name);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add '{}' to facility definition {}]\n", name, defId);
    }
    DWORD lastSendID;
    hr = SimConnect_GetLastSentPacketID(hSimConnect, &lastSendID);
    if (FAILED(hr)) {
        std::cerr << "[Failed to get last SendID]\n";
        lastSendID = 0;
    }
}


auto main(int argc, const char* argv[]) -> int
{
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
			}
			else {
				args[arg.substr(2)] = ""; // No value provided
			}
		}
		else {
			args["Arg" + std::to_string(fixedArg++)] = arg;
		}
	}

	std::string region{ "" };
    if (args.find("region") != args.end()) {
        region = args["region"];
	}

	std::string icao{ "EHGG" };
    if (args.find("icao") != args.end()) {
        icao = args["icao"];
    }
    else {
        std::cerr << std::format("No ICAO code provided, using default '{}'.\n", icao);
	}

	std::string parking{ "" };
    if (args.find("parking") != args.end()) {
        parking = args["parking"];
	}

    if (!connect()) {
		return -1;
	}
	std::cerr << "[Connected to MSFS 2020]\n";

    addToFacilityDef(DEFINITION_ID, "OPEN AIRPORT");

    addToFacilityDef(DEFINITION_ID, "ICAO");
    addToFacilityDef(DEFINITION_ID, "REGION");

    addToFacilityDef(DEFINITION_ID, "LATITUDE");
    addToFacilityDef(DEFINITION_ID, "LONGITUDE");
    addToFacilityDef(DEFINITION_ID, "ALTITUDE");

    addToFacilityDef(DEFINITION_ID, "OPEN TAXI_PARKING");
    addToFacilityDef(DEFINITION_ID, "TYPE");
    addToFacilityDef(DEFINITION_ID, "TAXI_POINT_TYPE");
    addToFacilityDef(DEFINITION_ID, "NAME");
    addToFacilityDef(DEFINITION_ID, "SUFFIX");
    addToFacilityDef(DEFINITION_ID, "NUMBER");
    addToFacilityDef(DEFINITION_ID, "ORIENTATION");
    addToFacilityDef(DEFINITION_ID, "HEADING");
    addToFacilityDef(DEFINITION_ID, "BIAS_X");
    addToFacilityDef(DEFINITION_ID, "BIAS_Z");
    addToFacilityDef(DEFINITION_ID, "CLOSE TAXI_PARKING");

    addToFacilityDef(DEFINITION_ID, "CLOSE AIRPORT");

    HRESULT hrRequest = SimConnect_RequestFacilityData(hSimConnect, DEFINITION_ID, REQUEST_ID, icao.c_str(), region.c_str());
    if (FAILED(hrRequest)) {
        std::cerr << std::format("Failed to request parking data for airport '{}'!\n", icao);
    }
    else {
        std::cerr << std::format("[Requested parking data for airport '{}'...]\n", icao);
    }
	handle_messages(icao, region, parking);

    disconnect();

	return 0;
}