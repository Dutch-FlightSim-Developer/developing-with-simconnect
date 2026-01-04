/*
 * Copyright (c) 2024. Bert Laverman
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

// The pure API in SimConnect is not clang-tidy-proof
// NOLINTBEGIN

#pragma warning(push, 3)
#include <windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#include <iostream>
#include <format>
#include <ctime>


/**
 * A shorthand test if we need to avoid using MSFS 2024 specific features.
 */
#if defined(SIMCONNECT_TYPEDEF)
#define MSFS_2024_SDK 1     // NOLINT(cppcoreguidelines-macro-usage)
#else
#define MSFS_2024_SDK 0     // NOLINT(cppcoreguidelines-macro-usage)
#endif


enum REQUEST_IDS : SIMCONNECT_DATA_REQUEST_ID{
	REQ_AIRCRAFT_LOADED,
	REQ_FLIGHT_LOADED,
	REQ_FLIGHTPLAN_LOADED,
	REQ_DIALOG_MODE,
	REQ_SIM_STATE,
	REQ_SIM_LOADED
};


static HANDLE hSimConnect{ nullptr };		// SimConnect handle


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


/**
 * Request the system state.
 * @param name The name of the system state
 */
static void requestSystemState(DWORD reqId, const char* name) {
	if (SUCCEEDED(SimConnect_RequestSystemState(hSimConnect, reqId, name))) {
		std::cerr << std::format("[SystemState '{}' requested with RequestID {}]\n", name, reqId);
	}
	else {
		std::cerr << std::format("[Request for '{}' AircraftLoaded failed]\n", name);
	}
}


static bool connected{ true };		// If `true`, we are currently connected to the simulator.


/**
 * Helper to convert a SIMCONNECT_RECV pointer to a more specific type.
 *
 * @tparam Recv The specific SIMCONNECT_RECV type to convert to.
 * @param ptr The pointer to convert.
 * @return The converted pointer.
 */
template <typename Recv>
inline const Recv* toRecvPtr(const void* ptr) { return reinterpret_cast<const Recv*>(ptr); }


/**
 * Process all currently available messages.
 */
static void processMessages() {
	while(connected) {
		SIMCONNECT_RECV* data{ nullptr };
		DWORD len{ 0 };
		HRESULT hr = SimConnect_GetNextDispatch(hSimConnect, &data, &len);

		if (FAILED(hr)) {
			break;
		}

		switch (data->dwID) {

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

		case SIMCONNECT_RECV_ID_SYSTEM_STATE:		// A system state has been received
		{
			SIMCONNECT_RECV_SYSTEM_STATE* msg = (SIMCONNECT_RECV_SYSTEM_STATE*)data;

            switch (msg->dwRequestID) {

            case REQ_AIRCRAFT_LOADED:
                std::cout << std::format("AircraftLoaded: '{}'\n", msg->szString);
                break;

            case REQ_FLIGHT_LOADED:
                std::cout << std::format("FlightLoaded: '{}'\n", msg->szString);
                break;

            case REQ_FLIGHTPLAN_LOADED:
                std::cout << std::format("FlightPlan: '{}'\n", msg->szString);
                break;

            case REQ_DIALOG_MODE:
                std::cout << std::format("DialogMode: {}\n", msg->dwInteger);
                break;

            case REQ_SIM_STATE:
                std::cout << std::format("Sim State: {}\n", msg->dwInteger);
                break;

            case REQ_SIM_LOADED:
                std::cout << std::format("Sim Loaded: '{}'\n", msg->szString);
                break;

            default:
                std::cerr << std::format("[Unknown systemState for request {} received. (dwInteger={}, fFloat={}, szString='{}')]\n",
                    msg->dwRequestID, msg->dwInteger, msg->fFloat, msg->szString);
                break;
            }

		}
			break;

		default:
			std::cerr << std::format("[Received an unknown message with type {}. (size {} bytes)]\n", data->dwID, len);
			break;
		}
	}
}


/**
 * The main entry point.
 * @param argc The number of command line arguments
 * @param argv The command line arguments
 * @return The exit code
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "RequestSystemState", nullptr, 0, nullptr, 0))) {
		std::cout << "Connected to the Simulator.\n";

		requestSystemState(REQ_AIRCRAFT_LOADED, "AircraftLoaded");
		requestSystemState(REQ_DIALOG_MODE, "DialogMode");
		requestSystemState(REQ_FLIGHT_LOADED, "FlightLoaded");
		requestSystemState(REQ_FLIGHTPLAN_LOADED, "FlightPlan");
		requestSystemState(REQ_SIM_STATE, "Sim");
		requestSystemState(REQ_SIM_LOADED, "SimLoaded"); // Will cause an exception

		std::cout << "Handling messages for 10 seconds.\n";
		const time_t start{ time(nullptr) };

		while (connected && (difftime(time(nullptr), start) < 10)) {
			processMessages();

			if (connected) {
				Sleep(100);		// Try to convince our protection we're not malware
			}
		}

		SimConnect_Close(hSimConnect);
		std::cout << "Disconnected from the simulator.\n";
	}
	else {
		std::cerr << "Failed to connect to the simulator.\n";
	}
	return 0;
}

// NOLINTEND