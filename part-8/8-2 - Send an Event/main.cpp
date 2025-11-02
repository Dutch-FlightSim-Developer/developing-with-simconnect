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
#include <chrono>
#include <map>

using namespace std::chrono_literals;


constexpr static const char* appName = "Receiving events";
static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };

static std::map<std::string, std::string> args;

constexpr static SIMCONNECT_INPUT_GROUP_ID INPGRP_EXIT{ 1 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVT_EXIT{ 1 };

constexpr static SIMCONNECT_NOTIFICATION_GROUP_ID GROUP{ 2 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVENT{ 2 };


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


/**
 * Handle position update messages from SimConnect.
 */
static void handleMessages(std::chrono::seconds duration)
{
    bool haveNoDeadline{ duration.count() == 0 };
    if (haveNoDeadline) {
        std::cerr << "[Handling messages until stopped]\n";
    }
    else {
        std::cerr << std::format("[Handling messages for {} seconds]\n", duration.count());
    }

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point endTime = startTime + duration;

    while (haveNoDeadline || (std::chrono::steady_clock::now() <= endTime)) {
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

        for (HRESULT hr{ S_OK }; (haveNoDeadline || (std::chrono::steady_clock::now() <= endTime)) && SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData)); Sleep(100)) {
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

            case SIMCONNECT_RECV_ID_EVENT:
            case SIMCONNECT_RECV_ID_EVENT_EX1:
            {
                const SIMCONNECT_RECV_EVENT* pEvent = toRecvPtr<SIMCONNECT_RECV_EVENT>(pData);
                if (pEvent->uEventID == EVT_EXIT) {
                    std::cerr << "[Exit event received, shutting down]\n";
                    return;
                }
                else {
                    std::cerr << std::format("[Unknown event ID received: {} with data {}]\n", pEvent->uEventID, pEvent->dwData);
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
    HRESULT hr;

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


static bool sendEvent(const std::string& eventName, DWORD value)
{
    HRESULT hr;
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT, eventName.c_str());
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map event '{}': HRESULT 0x{:08X}]\n", eventName, hr);
        return false;
    }
    DWORD sendId{ 0 };
	hr = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to get last sent packet ID: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
	std::cerr << std::format("[SendID for mapping '{}' to {} is {}]\n", eventName, value, sendId);

    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP, EVENT);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add event '{}' to notification group: HRESULT 0x{:08X}]\n", eventName, hr);
        return false;
	}
	hr = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to get last sent packet ID: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
	std::cerr << std::format("[SendID for adding client event {} to group {} is {}]\n", value, GROUP, sendId);

    hr = SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT, value, GROUP, SIMCONNECT_EVENT_FLAG_DEFAULT);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to transmit event '{}' with value '{}': HRESULT 0x{:08X}]\n", eventName, value, hr);
        return false;
	}
    hr = SimConnect_GetLastSentPacketID(hSimConnect, &sendId);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to get last sent packet ID: HRESULT 0x{:08X}]\n", hr);
        return false;
    }
    std::cerr << std::format("[SendID for transmitting client event {} with parameter value {} is {}]\n", EVENT, value, sendId);

    return true;
}


auto main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) -> int
{
    gatherArgs(argc, argv);

    if (!args.contains("Arg1")) {
        std::cerr << "[ABORTING: No event name specified.]\n";
        return 1;
	}
	std::string eventName = args["Arg1"];
    DWORD eventValue{ 0 };
    if (args.contains("Arg2")) {
        // allow hex values
        if (args["Arg2"].starts_with("0x") || args["Arg2"].starts_with("0X")) {
            eventValue = std::stoul(args["Arg2"], nullptr, 16);
        }
        else {
            eventValue = std::stoul(args["Arg2"]);
		}
    }

	std::cerr << std::format("Going to transmit event '{}' with value '0x{:04x}'\n", eventName, eventValue);

    if (!connect()) {
        return -1;
    }
    std::cout << "Connected to MSFS 2020!\n";

    // Set up keyboard input if requested
    if (!setupKeys()) {
        std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
        return 1;
    }
    // Subscribe to flap-related events
    if (!sendEvent(eventName, eventValue)) {
        std::cerr << "[ABORTING: Failed to subscribe to flap-related events]\n";
        return 1;
    }

    // Handle messages for a second, that should suffice.
    handleMessages(1s);

    disconnect();

    return 0;
}