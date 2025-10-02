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
#include <fstream>
#include <iomanip>
#include <locale>
#include <format>
#include <string>
#include <map>
#include <vector>
#include <cstring>

#include <cmath>
#include <numbers>
#include <chrono>


constexpr static const char* appName = "List parkings";
static HANDLE hSimConnect{ nullptr };		// The connection handle
static HANDLE hEvent{ nullptr };			// The event handle (for efficient waiting for SimConnect messages)

static std::map<std::string, std::string> args;

// Map to store SendIDs for tracking
static std::map<DWORD, std::string> sendIdTracker;

constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_CREATE_AIRCRAFT{ 1 };
constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_ONGROUND{ 2 };
constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_SET_POSITION{ 3 };

constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_ONGROUND{ 1 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_POSITION{ 2 };

constexpr static SIMCONNECT_INPUT_GROUP_ID INPGRP_RECORD{ 1 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVT_TOGGLE_RECORDING{ 1 };
constexpr static SIMCONNECT_INPUT_GROUP_ID INPGRP_EXIT{ 2 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVT_EXIT{ 2 };


/**
 * Add a data field to a SimConnect data definition with SendID tracking and optional logging.
 *
 * @param definitionId The data definition ID to add the field to.
 * @param datumName The name of the data field (e.g., "PLANE LATITUDE").
 * @param unitsName The units for the data field (can be nullptr).
 * @param dataType The SimConnect data type.
 * @param fieldName A human-readable name for the field for logging purposes.
 * @param enableLogging Whether to log the operation.
 */
static bool addDataDefinitionField(SIMCONNECT_DATA_DEFINITION_ID definitionId, const char* datumName, const char* unitsName, SIMCONNECT_DATATYPE dataType, const std::string& fieldName, float epsilon = 0.0f)
{
    HRESULT hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, datumName, unitsName, dataType, epsilon);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add '{}' to data definition {}: HRESULT 0x{:08X}]\n", fieldName, definitionId, hr);
        return false;
    }

    DWORD sendId{ 0 };
    hr = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to get last sent packet ID after adding '{}' to data definition {}: HRESULT 0x{:08X}]\n", fieldName, definitionId, hr);
        return false;
    }

    // Store the mapping for tracking
    sendIdTracker[sendId] = std::format("AddToDataDefinition: {} ({})", fieldName, datumName);

    return true;
}


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

        // Look up the SendID in our tracker
        auto it = sendIdTracker.find(msg.dwSendID);
        if (it != sendIdTracker.end()) {
            printf("- SendID %u corresponds to: %s\n", msg.dwSendID, it->second.c_str());
        }
        else {
            printf("- SendID %u not found in tracker.\n", msg.dwSendID);
        }
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
struct AircraftInfo {
    std::string category{ "" };             // CATEGORY (String, fixed list but std::string is more flexible)
    std::string title{ "" };                // TITLE (String, max 128 chars)
    std::string livery{ "" };               // LIVERY NAME (String, max 128 chars)
    std::string atcId{ "" };                // ATC ID (String, max 10 chars)
    std::string atcModel{ "" };             // ATC MODEL (String, max 128 chars)

    int32_t isUserAircraft{ 1 };            // IS USER SIM (Bool, defaulting to 32-bit int 0 or 1)

    double planeLatitude{ 0.0 };            // PLANE ALTITUDE (Degrees)
    double planeLongitude{ 0.0 };           // PLANE LONGITUDE (Degrees)
    double planeAltitude{ 0.0 };            // PLANE ALTITUDE (Feet)

    float planePitch{ 0.0f };               // PLANE PITCH DEGREES (Degrees)
    float planeBank{ 0.0f };                // PLANE BANK DEGREES (Degrees)
    float planeHeading{ 0.0f };             // PLANE HEADING DEGREES TRUE (Degrees)

    int32_t onGround{ 0 };                  // SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)

    float planeAirspeed{ 0.0f };            // AIRSPEED TRUE (Knots)
};

struct AircraftPosition {
	long timestamp{ 0 };

    double planeLatitude{ 0.0 };           // PLANE ALTITUDE (Degrees)
    double planeLongitude{ 0.0 };          // PLANE LONGITUDE (Degrees)
    double planeAltitude{ 0.0 };           // PLANE ALTITUDE (Feet)

    float planePitch{ 0.0f };               // PLANE PITCH DEGREES (Degrees)
    float planeBank{ 0.0f };                // PLANE BANK DEGREES (Degrees)
    float planeHeading{ 0.0f };             // PLANE HEADING DEGREES TRUE (Degrees)

    float planeAirspeed{ 0.0f };            // AIRSPEED INDICATED (Knots)

    float planeVelocityX{ 0.0f };           // VELOCITY BODY X (Feet per second)
    float planeVelocityY{ 0.0f };           // VELOCITY BODY Y (Feet per second)
    float planeVelocityZ{ 0.0f };           // VELOCITY BODY Z (Feet per second)

    float planeAccelerationX{ 0.0f };       // ACCELERATION BODY X (Feet per second squared)
    float planeAccelerationY{ 0.0f };       // ACCELERATION BODY Y (Feet per second squared)
    float planeAccelerationZ{ 0.0f };       // ACCELERATION BODY Z (Feet per second squared)

    float planeRotationVelocityX{ 0.0f };   // ROTATION VELOCITY BODY X (Degrees per second)
    float planeRotationVelocityY{ 0.0f };   // ROTATION VELOCITY BODY Y (Degrees per second)
    float planeRotationVelocityZ{ 0.0f };   // ROTATION VELOCITY BODY Z (Degrees per second)
};
#pragma pack(pop) // Restore previous packing alignment


static AircraftInfo aircraftInfo;
static DWORD aircraftId{ 0 };
static std::vector<AircraftPosition> aircraftPositions;
static DWORD dataSize{ 0 };


/**
 * First variation: Define aircraft position with only location and attitude.
 */
static bool defineAircraftPositionLocationOnly() {
	dataSize = 3 * sizeof(double) + 3 * sizeof(float);

    return addDataDefinitionField(DEFID_POSITION, "PLANE LATITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Latitude", 0.0001f) &&
           addDataDefinitionField(DEFID_POSITION, "PLANE LONGITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Longitude", 0.0001f) &&
           addDataDefinitionField(DEFID_POSITION, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT64, "Plane Altitude", 0.0001f) &&
           addDataDefinitionField(DEFID_POSITION, "PLANE PITCH DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Pitch", 0.0001f) &&
           addDataDefinitionField(DEFID_POSITION, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Bank", 0.0001f) &&
           addDataDefinitionField(DEFID_POSITION, "PLANE HEADING DEGREES TRUE", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Heading", 0.0001f);
}


/**
 * Second variation: Define aircraft position with location, attitude, and speed.
 */
static bool defineAircraftPositionWithSpeed() {
	dataSize = 3 * sizeof(double) + 7 * sizeof(float);
    return addDataDefinitionField(DEFID_POSITION, "PLANE LATITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Latitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE LONGITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Longitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT64, "Plane Altitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE PITCH DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Pitch", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Bank", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE HEADING DEGREES TRUE", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Heading", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "AIRSPEED INDICATED", "knots", SIMCONNECT_DATATYPE_FLOAT32, "Airspeed Indicated", 0.1f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY X", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body X", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY Y", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body Y", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY Z", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body Z", 0.01f);
}


/**
 * Third variation: Define aircraft position with full data set.
 */
static bool defineAircraftPositionFull() {
    return addDataDefinitionField(DEFID_POSITION, "PLANE LATITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Latitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE LONGITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Longitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT64, "Plane Altitude", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE PITCH DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Pitch", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Bank", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "PLANE HEADING DEGREES TRUE", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Heading", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "AIRSPEED INDICATED", "knots", SIMCONNECT_DATATYPE_FLOAT32, "Airspeed Indicated", 0.1f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY X", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body X", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY Y", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body Y", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "VELOCITY BODY Z", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity Body Z", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "ACCELERATION BODY X", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration Body X", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "ACCELERATION BODY Y", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration Body Y", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "ACCELERATION BODY Z", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration Body Z", 0.01f) &&
        addDataDefinitionField(DEFID_POSITION, "ROTATION VELOCITY BODY X", "degrees per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body X", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "ROTATION VELOCITY BODY Y", "degrees per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body Y", 0.0001f) &&
        addDataDefinitionField(DEFID_POSITION, "ROTATION VELOCITY BODY Z", "degrees per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body Z", 0.0001f);
}


/**
 * Trim leading and trailing whitespace from a string.
 *
 * @param str The string to trim.
 * @return The trimmed string.
 */
static std::string trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return ""; // All whitespace
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}


/**
 * Parse a string value, removing surrounding quotes if present.
 *
 * @param value The string value to parse.
 * @return The parsed string without surrounding quotes.
 */
static std::string parseStringValue(const std::string& value) {
    std::string trimmed = trim(value);
    if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
        return trimmed.substr(1, trimmed.size() - 2);
    }
    return trimmed;
}


/**
 * Load the aircraft info from a YAML file.
 *
 * @param filename The name of the file to read from.
 * @return true if the file was loaded successfully, false otherwise.
 */
static bool loadAircraftInfo(std::string filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("[Failed to open file '{}' for reading]\n", filename);
        return false;
    }

    using namespace std::chrono;

    ifs.imbue(std::locale::classic());

    // Initialize aircraftInfo with default values
    aircraftInfo = AircraftInfo{};

    std::string line;
    std::string currentSection;

    while (std::getline(ifs, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check for section headers
        if (line == "aircraft:") {
            currentSection = "aircraft";
            continue;
        }
        else if (line == "initial-position:") {
            currentSection = "initial-position";
            continue;
        }
        else if (line == "metadata:") {
            currentSection = "metadata";
            continue;
        }

        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));

        // Remove leading spaces/tabs for indented keys
        if (key.starts_with("  ")) {
            key = key.substr(2);
        }

        try {
            if (currentSection == "aircraft") {
                if (key == "category") {
                    aircraftInfo.category = parseStringValue(value);
                }
                else if (key == "title") {
                    aircraftInfo.title = parseStringValue(value);
                }
                else if (key == "livery") {
                    aircraftInfo.livery = parseStringValue(value);
                }
                else if (key == "atc-id") {
                    aircraftInfo.atcId = parseStringValue(value);
                }
                else if (key == "atc-model") {
                    aircraftInfo.atcModel = parseStringValue(value);
                }
                else if (key == "is-user") {
                    aircraftInfo.isUserAircraft = std::stoi(value);
                }
            }
            else if (currentSection == "initial-position") {
                if (key == "latitude") {
                    aircraftInfo.planeLatitude = std::stod(value);
                }
                else if (key == "longitude") {
                    aircraftInfo.planeLongitude = std::stod(value);
                }
                else if (key == "altitude") {
                    aircraftInfo.planeAltitude = std::stod(value);
                }
                else if (key == "pitch") {
                    aircraftInfo.planePitch = std::stof(value);
                }
                else if (key == "bank") {
                    aircraftInfo.planeBank = std::stof(value);
                }
                else if (key == "heading") {
                    aircraftInfo.planeHeading = std::stof(value);
                }
                else if (key == "on-ground") {
                    aircraftInfo.onGround = (value == "true" || value == "1") ? 1 : 0;
                }
                else if (key == "air-speed") {
                    aircraftInfo.planeAirspeed = std::stof(value);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << std::format("[Warning: Failed to parse value '{}' for key '{}': {}]\n", value, key, e.what());
            return false;
        }
    }

    std::cerr << std::format("[Aircraft info loaded from '{}']\n", filename);
    std::cerr << std::format("[Loaded: category='{}', title='{}', livery='{}', atc-id='{}', is-user={}]\n",
        aircraftInfo.category, aircraftInfo.title, aircraftInfo.livery, aircraftInfo.atcId, aircraftInfo.isUserAircraft);

    return true;
}


/**
 * Load the aircraft position data from a YAML file into memory.
 */
static bool loadPositionData(std::string filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        std::cerr << std::format("[Failed to open file '{}' for reading]\n", filename);
        return false;
    }
    ifs.imbue(std::locale::classic());

    std::string line;
	AircraftPosition position{};
	bool firstPosition{ true };

    while (std::getline(ifs, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Handle the preamble
        if (line.starts_with("kind:")) {
			std::cerr << std::format("[Loading a file of kind '{}']\n", trim(line.substr(5)));
			continue;
        }
        if (line.starts_with("simulator:")) {
            std::cerr << std::format("[File was recorded with {}]\n", parseStringValue(trim(line.substr(11))));
            continue;
        }

        // Anything left to ignore...
        if (line.starts_with("metadata:") || line.starts_with("start-time:") || line.starts_with("positions:")) {
            continue; // Skip the unused bits
		}

        if (line.starts_with("- ")) {
			std::cerr << "."; // Progress indicator
            if (!firstPosition) {
                aircraftPositions.push_back(position);
                position = AircraftPosition{};
				line = trim(line.substr(2)); // Remove "- " prefix
            }
            else {
                firstPosition = false;
            }
        }
        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));

        try {
            if (key == "msecs") {
                position.timestamp = std::stol(value);
            }
            else if (key == "latitude") {
                position.planeLatitude = std::stod(value);
            }
            else if (key == "longitude") {
                position.planeLongitude = std::stod(value);
            }
            else if (key == "altitude") {
                position.planeAltitude = std::stod(value);
            }
            else if (key == "pitch") {
                position.planePitch = std::stof(value);
            }
            else if (key == "bank") {
                position.planeBank = std::stof(value);
            }
            else if (key == "heading") {
                position.planeHeading = std::stof(value);
            }
            else if (key == "air-speed") {
                position.planeAirspeed = std::stof(value);
            }
            else if (key == "velocity-x") {
                position.planeVelocityX = std::stof(value);
            }
            else if (key == "velocity-y") {
                position.planeVelocityY = std::stof(value);
            }
            else if (key == "velocity-z") {
                position.planeVelocityZ = std::stof(value);
            }
            else if (key == "acceleration-x") {
                position.planeAccelerationX = std::stof(value);
            }
            else if (key == "acceleration-y") {
                position.planeAccelerationY = std::stof(value);
            }
            else if (key == "acceleration-z") {
                position.planeAccelerationZ = std::stof(value);
            }
            else if (key == "rotation-velocity-x") {
                position.planeRotationVelocityX = std::stof(value);
            }
            else if (key == "rotation-velocity-y") {
                position.planeRotationVelocityY = std::stof(value);
            }
            else if (key == "rotation-velocity-z") {
                position.planeRotationVelocityZ = std::stof(value);
			}
        }
        catch (const std::exception& e) {
            std::cerr << std::format("[Warning: Failed to parse value '{}' for key '{}': {}]\n", value, key, e.what());
            return false;
        }
    }
    if (position.timestamp != 0) {
        aircraftPositions.push_back(position);
    }
    std::cerr << ".\n";
	std::cerr << std::format("[Loaded {} position entries from '{}']\n", aircraftPositions.size(), filename);
    return true;
}


/**
 * Sleep for a short duration if connected.
 *
 * @param connected Whether the connection is active.
 */
static void sleepIfConnected(bool connected) {
    if (connected) {
        Sleep(100);
    }
}


/**
 * Force the specified object to be on the ground. This to ensure the aircraft (or other vehicle) is
 * not floating above or below the ground-level. Note MSFS 2024 has a bug where SimObjects can still
 * end up being misaligned with the ground even due to miscalculations wrt to the gear height.
 */
static void setOnGround(DWORD objectId) {
    HRESULT hrSet = SimConnect_AddToDataDefinition(hSimConnect, DEFID_ONGROUND, "SIM ON GROUND", "Bool", SIMCONNECT_DATATYPE_INT32, 0.0f, 0);
    if (FAILED(hrSet)) {
        std::cerr << std::format("[Failed to add to data definition DEFID_ONGROUND: 0x{:08X}]\n", hrSet);
        return;
    }
    DWORD sendId{ 0 };
    hrSet = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (SUCCEEDED(hrSet)) {
        sendIdTracker[sendId] = std::format("Add 'SIM SHOULD SET ON GROUND' state for definition block {} on SimObject ID {}.", DEFID_ONGROUND, objectId);
    }
    else {
        std::cerr << std::format("[Failed to get SendID for call to AddToDataDefinition(): 0x{:08X}]\n", hrSet);
    }
    int32_t onGroundValue{ 1 };
    hrSet = SimConnect_SetDataOnSimObject(hSimConnect, DEFID_ONGROUND, objectId, SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0, sizeof(onGroundValue), &onGroundValue);
    if (FAILED(hrSet)) {
        std::cerr << std::format("[Failed to set on-ground state for Object ID {}: 0x{:08X}]\n", objectId, hrSet);
        return;
    }
    hrSet = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (SUCCEEDED(hrSet)) {
        sendIdTracker[sendId] = std::format("Requested SimObject ID {} to be forced 'on ground'.", objectId);
    }
    else {
        std::cerr << std::format("[Failed to get SendID for call to AddToDataDefinition(): 0x{:08X}]\n", hrSet);
    }
    std::cerr << std::format("[Set AI Aircraft Object ID {} to be on-ground]\n", objectId);
}


/**
 * Handle messages from SimConnect.
 */
static void handleMessages(std::chrono::seconds deadline)
{
    bool processing{ true };
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point endTime = startTime + deadline;

    size_t positionIndex{ 0 };
    std::chrono::steady_clock::time_point replayStart{ startTime };

    while (processing && (std::chrono::steady_clock::now() <= endTime)) {
        if ((aircraftId != 0) && (replayStart != startTime) && (positionIndex < aircraftPositions.size())) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - replayStart;
            long elapsedMillis = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
            // Send all positions that are due
            while ((positionIndex < aircraftPositions.size()) && (aircraftPositions[positionIndex].timestamp <= elapsedMillis)) {
                const AircraftPosition& pos = aircraftPositions[positionIndex];
                std::cerr << std::format("[Sending position #{}]\n", positionIndex + 1);
                HRESULT hrSet = SimConnect_SetDataOnSimObject(hSimConnect, DEFID_POSITION, aircraftId, SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0, dataSize, const_cast<void*>(static_cast<const void*>(&(pos.planeLatitude))));
                if (FAILED(hrSet)) {
                    std::cerr << std::format("[Failed to set position data for Object ID {}: 0x{:08X}]\n", aircraftId, hrSet);
                    return;
                }
                DWORD sendId{ 0 };
                hrSet = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
                if (SUCCEEDED(hrSet)) {
                    sendIdTracker[sendId] = std::format("Sent position update #{} to SimObject ID {}.", positionIndex + 1, aircraftId);
                }
                else {
                    std::cerr << std::format("[Failed to get SendID for call to SetDataOnSimObject(): 0x{:08X}]\n", hrSet);
                }
                positionIndex++;
                std::cerr << std::format("[Next position due in {} ms]\n",
                    (positionIndex < aircraftPositions.size()) ?
                    (aircraftPositions[positionIndex].timestamp - elapsedMillis) : 0);
            }
        }

        auto waitResult = ::WaitForSingleObject(hEvent, 100);
        if (waitResult == WAIT_TIMEOUT) {
            continue; // Timeout, loop again to check the time
        }
        if (waitResult < 0) {
            std::cerr << std::format("[WaitForSingleObject failed: 0x{:08X}]\n", GetLastError());
            return;
        }
        if (waitResult != WAIT_OBJECT_0) {
            std::cerr << std::format("[Unexpected WaitForSingleObject result: {}]\n", waitResult);
        }
        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };

        for (HRESULT hr{ S_OK }; (std::chrono::steady_clock::now() <= endTime) && SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData)); sleepIfConnected(true)) {
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
                std::cerr << "[Simulator is shutting down]\n";
                processing = false;
            }
            break;

            case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
            {
                const SIMCONNECT_RECV_ASSIGNED_OBJECT_ID* pObj = toRecvPtr<SIMCONNECT_RECV_ASSIGNED_OBJECT_ID>(pData);
                if (pObj->dwRequestID == REQID_CREATE_AIRCRAFT) {
                    std::cerr << std::format("[AI Aircraft created with Object ID {}]\n", pObj->dwObjectID);
					aircraftId = pObj->dwObjectID;

                    if (args.contains("onground") && (aircraftInfo.onGround == 1)) {
						setOnGround(aircraftId);
                    }
                }
                else {
                    std::cerr << std::format("[Received ASSIGNED_OBJECT_ID for unknown Request ID {}: Object ID {}]\n", pObj->dwRequestID, pObj->dwObjectID);
                }
            }
            break;

            case SIMCONNECT_RECV_ID_EVENT:
            {
                const SIMCONNECT_RECV_EVENT* pEvent = toRecvPtr<SIMCONNECT_RECV_EVENT>(pData);
				std::cerr << std::format("[Received event ID {} for Group ID {}]\n", pEvent->uEventID, pEvent->uGroupID);
                if (pEvent->uEventID == EVT_TOGGLE_RECORDING) {
                    if (replayStart == startTime) {
                        std::cerr << std::format("[Starting replay, first position due in {} ms]\n", aircraftPositions[0].timestamp);
                        replayStart = std::chrono::steady_clock::now();
                    }
                    else {
                        std::cerr << "[Already running replay]\n";
                    }
                }
                else if (pEvent->uEventID == EVT_EXIT) {
                    std::cerr << "[Exit event received, shutting down]\n";
                    return;
                }
            }
            break;

            default:
            {
                std::cerr << std::format("[Ignoring message of type {} (length {} bytes)]\n", pData->dwID, pData->dwSize);
            }
            break;
            }
        }

    }
}


/**
 * Set up keyboard input to toggle recording and exit the program.
 */
static bool setupKeys()
{
    HRESULT hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVT_TOGGLE_RECORDING, "Toggle.Recording");
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map client event to sim event: HRESULT 0x{:08X}]\n", hr);
        return false;
    }

    hr = SimConnect_MapInputEventToClientEvent_EX1(hSimConnect, INPGRP_RECORD, "VK_MEDIA_PLAY_PAUSE", EVT_TOGGLE_RECORDING);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map input event to client event: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_SetInputGroupState(hSimConnect, INPGRP_RECORD, SIMCONNECT_STATE_ON);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to enable input group: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, INPGRP_RECORD, EVT_TOGGLE_RECORDING);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add client event to notification group: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_SetNotificationGroupPriority(hSimConnect, INPGRP_RECORD, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to set notification group priority: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    std::cerr << "[Press the Play/Pause media key to toggle recording]\n";

    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVT_EXIT, "Exit.Program");
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map client event to sim event: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_MapInputEventToClientEvent_EX1(hSimConnect, INPGRP_EXIT, "VK_MEDIA_STOP", EVT_EXIT);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map input event to client event: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_SetInputGroupState(hSimConnect, INPGRP_EXIT, SIMCONNECT_STATE_ON);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to enable input group: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, INPGRP_EXIT, EVT_EXIT);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add client event to notification group: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    hr = SimConnect_SetNotificationGroupPriority(hSimConnect, INPGRP_EXIT, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to set notification group priority: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    std::cerr << "[Press the Escape key to exit the program]\n";
    return true;
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
 */
static void gatherArgs(int argc, const char* argv[])
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
}


auto main(int argc, const char* argv[]) -> int
{
    gatherArgs(argc, argv);

    std::chrono::seconds runDuration{ 60 }; // Default to 1 minute
    if (args.contains("duration")) {
        try {
            runDuration = std::chrono::seconds(std::stoi(args["duration"]));
        }
        catch (const std::exception&) {
            std::cerr << std::format("[Invalid duration '{}', using default of 60 seconds]\n", args["duration"]);
            runDuration = std::chrono::seconds(60);
        }
    }
    std::string filename{ "aircraft_info.yaml" };
    if (args.contains("Arg1")) {
        filename = args["Arg1"];
    }
    if (!loadAircraftInfo(filename)) {
        std::cerr << "[Failed to load aircraft info]\n";
		return 1;
    }
    if (!loadPositionData("aircraft_position_1.yaml")) {
		std::cerr << "[Failed to load aircraft position data]\n";
		return 1;
    }
	std::cout << std::format("[Loaded {} position entries]\n", aircraftPositions.size());

    if (!connect()) {
        std::cerr << "[ABORTING: Failed to connect to simulator]\n";
        return 1;
    }

    if (!setupKeys()) {
        std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
        return 1;
    }

    if (args.contains("full")) {
        std::cerr << "[Using full position data definition]\n";
        if (!defineAircraftPositionFull()) {
            std::cerr << "[Failed to define aircraft location data]\n";
            return 1;
        }
    }
    else if (args.contains("with-speed")) {
        std::cerr << "[Using position data definition with speed]\n";
        if (!defineAircraftPositionWithSpeed()) {
            std::cerr << "[Failed to define aircraft location data]\n";
            return 1;
        }
    }
    else {
        std::cerr << "[Using position data definition with location only]\n";
        if (!defineAircraftPositionLocationOnly()) {
            std::cerr << "[Failed to define aircraft location data]\n";
            return 1;
        }
    }
    HRESULT hr = SimConnect_AICreateNonATCAircraft_EX1(hSimConnect,
        aircraftInfo.title.c_str(),
        aircraftInfo.livery.c_str(),
        aircraftInfo.atcId.c_str(),
        SIMCONNECT_DATA_INITPOSITION{
            aircraftInfo.planeLatitude,
            aircraftInfo.planeLongitude,
            aircraftInfo.planeAltitude,
            aircraftInfo.planePitch,
            aircraftInfo.planeBank,
            aircraftInfo.planeHeading,
            static_cast<DWORD>(aircraftInfo.onGround),
            static_cast<DWORD>(aircraftInfo.planeAirspeed)
        },
        REQID_CREATE_AIRCRAFT);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to create AI aircraft: 0x{:08X}]\n", hr);
        disconnect();
    }
    handleMessages(runDuration);

    disconnect();

    return 0;
}