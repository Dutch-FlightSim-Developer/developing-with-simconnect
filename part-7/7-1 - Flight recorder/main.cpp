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

#include <cmath>
#include <numbers>
#include <chrono>


constexpr static const char* appName = "List parkings";
static HANDLE hSimConnect{ nullptr };		// The connection handle
static HANDLE hEvent{ nullptr };			// The event handle (for efficient waiting for SimConnect messages)

static std::map<std::string, std::string> args;

// Map to store SendIDs for tracking
static std::map<DWORD, std::string> sendIdTracker;
static DWORD nextSendId = 1000; // Start from 1000 to avoid conflicts with existing IDs

constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_AIRCRAFT_INFO{ 1 };
constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_AIRCRAFT_POSITION{ 2 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_AIRCRAFT_INFO{ 1 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_AIRCRAFT_POSITION{ 2 };

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
    char category[32];              // CATEGORY (String, fixed list but 32 should do)
    char title[128];                // TITLE (String, max 128 chars)
	char livery[256];               // LIVERY NAME (String, max 128 chars)
    char atcId[32];                 // ATC ID (String, max 10 chars)
	char atcModel[128];             // ATC MODEL (String, max 128 chars)

    int32_t isUserAircraft;         // IS USER SIM (Bool, defaulting to 32-bit int 0 or 1)

    double planeLatitude;           // PLANE ALTITUDE (Degrees)
    double planeLongitude;          // PLANE LONGITUDE (Degrees)
    double planeAltitude;           // PLANE ALTITUDE (Feet)

    float planePitch;               // PLANE PITCH DEGREES (Degrees)
    float planeBank;                // PLANE BANK DEGREES (Degrees)
    float planeHeading;             // PLANE HEADING DEGREES TRUE (Degrees)

	int32_t onGround;               // SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)

    float planeAirspeed;            // AIRSPEED TRUE (Knots)
};
struct AircraftPosition {
    double planeLatitude;           // PLANE ALTITUDE (Radians)
	double planeLongitude;          // PLANE LONGITUDE (Radians)
	double planeAltitude;           // PLANE ALTITUDE (Feet)

    float planePitch;               // PLANE PITCH DEGREES (Radians!)
	float planeBank;                // PLANE BANK DEGREES (Radians!)
	float planeHeading;             // PLANE HEADING DEGREES TRUE (Radians!)
	
    float planeAirspeed;            // AIRSPEED INDICATED (Knots)

    float planeVelocityX;           // VELOCITY WORLD X (Feet per second)
	float planeVelocityY;           // VELOCITY WORLD Y (Feet per second)
	float planeVelocityZ;           // VELOCITY WORLD Z (Feet per second)

    float planeAccelerationX;       // ACCELERATION WORLD X (Feet per second squared)
	float planeAccelerationY;       // ACCELERATION WORLD Y (Feet per second squared)
	float planeAccelerationZ;       // ACCELERATION WORLD Z (Feet per second squared)

    float planeRotationVelocityX;   // ROTATION VELOCITY BODY X (Radians per second)
	float planeRotationVelocityY;   // ROTATION VELOCITY BODY Y (Radians per second)
	float planeRotationVelocityZ;   // ROTATION VELOCITY BODY Z (Radians per second)

    int32_t onGround;               // SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)
};
#pragma pack(pop) // Restore previous packing alignment


static AircraftInfo aircraftInfo;


inline static double radiansToDegrees(double radians) {
    return radians * (180.0 / std::numbers::pi);
}
inline static float feetToMeters(float feet) {
    return feet * 0.3048f;
}
inline static float feetPerSecondToKnots(float fps) {
    return fps * 0.592483801f;
}


/**
 * Write the aircraft info to a YAML file.
 *
 * @param filename The name of the file to write to.
 */
static void writeAircraftInfo(std::string filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << std::format("[Failed to open file '{}' for writing, skipping AircraftInfo write]\n", filename);
        return;
    }

    using namespace std::chrono;

	ofs.imbue(std::locale::classic());
    ofs << "kind: AircraftInfo\n"
        << "metadata:\n"
        << "  start-time: " << std::format("{:%FT%TZ}", floor<seconds>(system_clock::now())) << "\n"
        << "  simulator: \"MSFS2024\"\n"
        << "aircraft:\n"
        << "  category: \"" << aircraftInfo.category << "\"\n"
        << "  title: \"" << aircraftInfo.title << "\"\n"
        << "  livery: \"" << aircraftInfo.livery << "\"\n"
        << "  atc-id: \"" << aircraftInfo.atcId << "\"\n"
        << "  atc-model: \"" << aircraftInfo.atcModel << "\"\n"
        << "  is-user: " << (aircraftInfo.isUserAircraft != 0) << "\n"
        << "initial-position:\n"
        << "  latitude: " << aircraftInfo.planeLatitude << "\n"
        << "  longitude: " << aircraftInfo.planeLongitude << "\n"
        << "  altitude: " << aircraftInfo.planeAltitude << "\n"
        << "  pitch: " << aircraftInfo.planePitch << "\n"
        << "  bank: " << aircraftInfo.planeBank << "\n"
        << "  heading: " << aircraftInfo.planeHeading << "\n"
        << "  on-ground: " << ((aircraftInfo.onGround != 0) ? "true" : "false") << "\n"
        << "  air-speed: " << aircraftInfo.planeAirspeed << "\n";

    std::cerr << std::format("[Aircraft info saved to '{}']\n", filename);
}


/**
 * Define the data structure for the aircraft info.
 *
 * @return true if the definition was successful, false otherwise.
 */
static bool defineAircraftInfo() {
    return addDataDefinitionField(DEFID_AIRCRAFT_INFO, "CATEGORY", nullptr, SIMCONNECT_DATATYPE_STRING32, "Category") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "TITLE", nullptr, SIMCONNECT_DATATYPE_STRING128, "Title") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "LIVERY NAME", nullptr, SIMCONNECT_DATATYPE_STRING256, "Livery Name") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "ATC ID", nullptr, SIMCONNECT_DATATYPE_STRING32, "ATC ID") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "ATC MODEL", nullptr, SIMCONNECT_DATATYPE_STRING128, "ATC Model") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "IS USER SIM", "bool", SIMCONNECT_DATATYPE_INT32, "Is User Aircraft") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE LATITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Latitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE LONGITUDE", "degrees", SIMCONNECT_DATATYPE_FLOAT64, "Plane Longitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT64, "Plane Altitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE PITCH DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Pitch", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Bank", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "PLANE HEADING DEGREES TRUE", "degrees", SIMCONNECT_DATATYPE_FLOAT32, "Plane Heading", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "SIM ON GROUND", "bool", SIMCONNECT_DATATYPE_INT32, "Sim On Ground") &&
           addDataDefinitionField(DEFID_AIRCRAFT_INFO, "AIRSPEED TRUE", "knots", SIMCONNECT_DATATYPE_FLOAT32, "True Airspeed", 0.1f);
}


/**
 * Define the data structure for the aircraft position.
 *
 * @return true if the definition was successful, false otherwise.
 */
static bool defineAircraftPosition() {
    return addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE LATITUDE", "radians", SIMCONNECT_DATATYPE_FLOAT64, "Plane Latitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE LONGITUDE", "radians", SIMCONNECT_DATATYPE_FLOAT64, "Plane Longitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE ALTITUDE", "feet", SIMCONNECT_DATATYPE_FLOAT64, "Plane Altitude", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE PITCH DEGREES", "radians", SIMCONNECT_DATATYPE_FLOAT32, "Plane Pitch", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE BANK DEGREES", "radians", SIMCONNECT_DATATYPE_FLOAT32, "Plane Bank", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "PLANE HEADING DEGREES TRUE", "radians", SIMCONNECT_DATATYPE_FLOAT32, "Plane Heading", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "AIRSPEED INDICATED", "knots", SIMCONNECT_DATATYPE_FLOAT32, "Airspeed Indicated", 0.1f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "VELOCITY BODY X", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity World X", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "VELOCITY BODY Y", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity World Y", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "VELOCITY BODY Z", "feet per second", SIMCONNECT_DATATYPE_FLOAT32, "Velocity World Z", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ACCELERATION BODY X", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration World X", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ACCELERATION BODY Y", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration World Y", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ACCELERATION BODY Z", "feet per second squared", SIMCONNECT_DATATYPE_FLOAT32, "Acceleration World Z", 0.01f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ROTATION VELOCITY BODY X", "radians per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body X", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ROTATION VELOCITY BODY Y", "radians per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body Y", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "ROTATION VELOCITY BODY Z", "radians per second", SIMCONNECT_DATATYPE_FLOAT32, "Rotation Velocity Body Z", 0.0001f) &&
           addDataDefinitionField(DEFID_AIRCRAFT_POSITION, "SIM ON GROUND", "bool", SIMCONNECT_DATATYPE_INT32, "Sim On Ground");
}


static void sleepIfConnected(bool connected) {
    if (connected) {
        Sleep(100);
    }
}


/**
 * Handle AircraftInfo message from SimConnect.
 */
static void handleAircraftInfoMessage(std::string filename)
{
    bool processing{ true };
    while (processing && (::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)) {
        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };

        for (HRESULT hr{ S_OK }; processing && SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData)); sleepIfConnected(processing)) {
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

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
            {
                const SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = toRecvPtr<SIMCONNECT_RECV_SIMOBJECT_DATA>(pData);

                if ((pObjData->dwRequestID == REQID_AIRCRAFT_INFO) && (pObjData->dwDefineID == DEFID_AIRCRAFT_INFO)) {
                    const AircraftInfo* info = reinterpret_cast<const AircraftInfo*>(&(pObjData->dwData));
                    aircraftInfo = *info;
                    std::cerr << std::format("[Received aircraft info: '{}' ({}), livery '{}', atcId '{}', atcModel '{}', isUserAircraft={}]\n",
                        aircraftInfo.title,
                        aircraftInfo.category,
                        aircraftInfo.livery,
                        aircraftInfo.atcId,
                        aircraftInfo.atcModel,
                        aircraftInfo.isUserAircraft);
                    writeAircraftInfo(filename);

                    processing = false; // We got the AircraftInfo
                }
                else {
                    std::cerr << std::format("[Ignoring SIMOBJECT_DATA message for request ID {} and definition ID {}]\n", pObjData->dwRequestID, pObjData->dwDefineID);
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


static bool recordingActive{ false };
static int recordingSegment{ 0 };
static std::ofstream positionData;


static bool startPositionData(std::string filename)
{
    if (positionData.is_open()) {
        positionData.close();
    }
    positionData.open(filename);
    positionData.imbue(std::locale::classic());
    positionData
        << "kind: AircraftPosition\n"
        << "metadata:\n"
        << "  start-time: " << std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())) << "\n"
        << "  simulator: \"MSFS2024\"\n"
        << "positions:\n"
        << std::fixed << std::setprecision(6);
    if (!positionData) {
        std::cerr << "[Failed to open 'aircraft_position.yaml' for writing, skipping position updates]\n";
        positionData.setstate(std::ios::badbit);
		return false;
    }

    HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQID_AIRCRAFT_POSITION, DEFID_AIRCRAFT_POSITION, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT, SIMCONNECT_PERIOD_SECOND, SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to request aircraft position data: HRESULT 0x{:08X}, skipping position updates]\n", hr);
        positionData.close();

        return false;
	}
    std::cerr << "[Position data recording started]\n";
    recordingActive = true;

    return true;
}


static void stopPositionData()
{
    if (recordingActive) {
        if (positionData.is_open()) {
            positionData.close();
            std::cerr << "[Position data file closed]\n";
        }

        HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQID_AIRCRAFT_POSITION, DEFID_AIRCRAFT_POSITION, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT, SIMCONNECT_PERIOD_NEVER);
        if (FAILED(hr)) {
            std::cerr << std::format("[Failed to cancel aircraft position data request: HRESULT 0x{:08X}]\n", hr);
        }
		std::cerr << "[Position data stream stopped]\n";
        recordingActive = false;
    }
}


/**
 * Handle position update messages from SimConnect.
 */
static void handleAircraftPositionUpdates(std::chrono::seconds duration)
{
	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point endTime = startTime + duration;
    while ((duration.count() == 0) || (std::chrono::steady_clock::now() <= endTime)) {
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

        for (HRESULT hr{ S_OK }; ((duration.count() == 0) || (std::chrono::steady_clock::now() <= endTime)) && SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData)); sleepIfConnected(true)) {
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
                return;
            }
            break;

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
            {
                const SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = toRecvPtr<SIMCONNECT_RECV_SIMOBJECT_DATA>(pData);

                if ((pObjData->dwRequestID == REQID_AIRCRAFT_POSITION) && (pObjData->dwDefineID == DEFID_AIRCRAFT_POSITION) && recordingActive) {
                    const AircraftPosition* pos = reinterpret_cast<const AircraftPosition*>(&(pObjData->dwData));

                    positionData
						<< "- msecs: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() << "\n"
                        << "  latitude: " << pos->planeLatitude << "\n"
                        << "  longitude: " << pos->planeLongitude << "\n"
                        << "  altitude: " << pos->planeAltitude << "\n"
                        << "  pitch: " << pos->planePitch << "\n"
                        << "  bank: " << pos->planeBank << "\n"
                        << "  heading: " << pos->planeHeading << "\n"
                        << "  airspeed: " << pos->planeAirspeed << "\n"
						<< "  velocity-x: " << pos->planeVelocityX << "\n"
						<< "  velocity-y: " << pos->planeVelocityY << "\n"
						<< "  velocity-z: " << pos->planeVelocityZ << "\n"
						<< "  acceleration-x: " << pos->planeAccelerationX << "\n"
						<< "  acceleration-y: " << pos->planeAccelerationY << "\n"
						<< "  acceleration-z: " << pos->planeAccelerationZ << "\n"
						<< "  rotation-velocity-x: " << pos->planeRotationVelocityX << "\n"
						<< "  rotation-velocity-y: " << pos->planeRotationVelocityY << "\n"
						<< "  rotation-velocity-z: " << pos->planeRotationVelocityZ << "\n"
                        << "  on-ground: " << ((pos->onGround != 0) ? "true" : "false") << "\n";
                }
            }
            break;

            case SIMCONNECT_RECV_ID_EVENT:
            {
                const SIMCONNECT_RECV_EVENT* pEvent = toRecvPtr<SIMCONNECT_RECV_EVENT>(pData);
                if (pEvent->uEventID == EVT_TOGGLE_RECORDING) {
                    if (recordingActive) {
                        stopPositionData();
                    }
                    else {
                        recordingSegment++;
						std::string filename = std::format("aircraft_position_{}.yaml", recordingSegment);
                        if (startPositionData(filename)) {
                            std::cerr << std::format("[Recording to '{}']\n", filename);
                        }
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
	std::chrono::seconds runDuration{ 0 }; // Default to 0, meaning don't record position data
    if (args.contains("duration")) {
        try {
            runDuration = std::chrono::seconds(std::stoi(args["duration"]));
        }
        catch (const std::exception&) {
            std::cerr << std::format("[Invalid duration '{}', using default of 60 seconds]\n", args["duration"]);
            runDuration = std::chrono::seconds(60);
        }
	}

    if (!connect()) {
        std::cerr << "[ABORTING: Failed to connect to simulator]\n";
        return 1;
    }

    if (args.contains("keyboard") && !setupKeys()) {
		std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
        return 1;
    }

    if (!defineAircraftInfo()) {
        disconnect();
		std::cerr << "[ABORTING: Failed to define aircraft info structure]\n";
		return 1;
    }
    if (!defineAircraftPosition()) {
		disconnect();
		std::cerr << "[ABORTING: Failed to define aircraft position structure]\n";
        return 1;
	}
    
    HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQID_AIRCRAFT_INFO, DEFID_AIRCRAFT_INFO, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT, SIMCONNECT_PERIOD_ONCE);
    if (FAILED(hr)) {
        disconnect();
        std::cerr << std::format("[ABORTING: Failed to request aircraft info: HRESULT 0x{:08X}]\n", hr);
        return 1;
	}
	handleAircraftInfoMessage("aircraft_info.yaml");

    if (runDuration.count() > 0) {
        if (!startPositionData("aircraft_position.yaml")) {
            disconnect();
            std::cerr << "[ABORTING: Failed to start position data recording]\n";
            return 1;
		}
    }
	handleAircraftPositionUpdates(runDuration);
	stopPositionData();

    disconnect();

    return 0;
}