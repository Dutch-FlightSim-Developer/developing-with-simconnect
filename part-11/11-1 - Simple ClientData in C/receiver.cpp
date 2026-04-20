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

#include "shared.h"


/**
 * A shorthand test if we need to avoid using MSFS 2024 specific features.
 */
#if defined(SIMCONNECT_TYPEDEF)
#define MSFS_2024_SDK 1     // NOLINT(cppcoreguidelines-macro-usage)
#else
#define MSFS_2024_SDK 0     // NOLINT(cppcoreguidelines-macro-usage)
#endif


using namespace std::chrono_literals;


constexpr static const char* appName = "ClientData Receiver";
static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };

// Must match the sender's CLIENT_DATA_ID — SimConnect resolves both by name
constexpr static SIMCONNECT_CLIENT_DATA_ID CLIENT_DATA_ID{ 1 };

// Must describe the same layout as the sender's definition
constexpr static SIMCONNECT_CLIENT_DATA_DEFINITION_ID DEF_ID{ 1 };

// Identifies this particular data request in incoming CLIENT_DATA messages
constexpr static SIMCONNECT_DATA_REQUEST_ID REQ_ID{ 1 };


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
static void handleException(const SIMCONNECT_RECV_EXCEPTION& msg)
{
    std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
    if (msg.dwSendID != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_SENDID) {
        std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
    }
    if (msg.dwIndex != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_INDEX) {
        std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex);
    }

    const SIMCONNECT_EXCEPTION exc{ static_cast<SIMCONNECT_EXCEPTION>(msg.dwException) };
    switch (exc) {
    case SIMCONNECT_EXCEPTION_NONE:
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
    case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:
        std::cerr << "The weather port is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:
        std::cerr << "The METAR string is invalid.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:
        std::cerr << "Unable to get the observation.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:
        std::cerr << "Unable to create the station.\n";
        break;
    case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:
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
    case SIMCONNECT_EXCEPTION_EVENT_NAME_RESERVED:
        std::cerr << "The event name is reserved and cannot be used.\n";
        break;
    case SIMCONNECT_EXCEPTION_INTERNAL:
        std::cerr << "An internal SimConnect error has occurred.\n";
        break;
    case SIMCONNECT_EXCEPTION_CAMERA_API:
        std::cerr << "An error related to the Camera API has occurred.\n";
        break;
#endif
        // No default; we want an error if we miss one
    }
}


/**
 * Connect to the simulator and create a Windows Event for async message handling.
 *
 * @return true if the connection was successful, false otherwise.
 */
static bool connect()
{
    if (hEvent == nullptr) {
        hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (hEvent == nullptr) {
            std::cerr << std::format("Failed to create event: 0x{:08X}.\n", GetLastError());
            return false;
        }
    }

    const HRESULT hr = SimConnect_Open(&hSimConnect, appName, nullptr, 0, hEvent, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("Failed to connect to SimConnect: 0x{:08X}\n", hr);
        return false;
    }

    return true;
}


/**
 * Disconnect from the simulator and close the Windows Event handle.
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
 * Helper to reinterpret a raw SIMCONNECT_RECV pointer as a more specific type.
 *
 * @tparam Recv The specific SIMCONNECT_RECV type to convert to.
 * @param ptr   The raw pointer to convert.
 * @return      The converted pointer.
 */
template <typename Recv>
inline const Recv* toRecvPtr(const void* ptr) { return reinterpret_cast<const Recv*>(ptr); }


/**
 * Subscribe to the shared client data area.
 *
 * The receiver does NOT create the data area — the sender owns it.
 * The receiver only needs to:
 *  1. Map the same well-known name to the same numeric ID.
 *  2. Describe the same data layout.
 *  3. Request delivery whenever the sender writes new data.
 *
 * @return true if subscription succeeded, false otherwise.
 */
static bool subscribeClientData()
{
    HRESULT hr;

    // Step 1 — resolve the well-known name to our local ID
    hr = SimConnect_MapClientDataNameToID(hSimConnect, CLIENT_DATA_NAME, CLIENT_DATA_ID);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to map client data name '{}': 0x{:08X}]\n", CLIENT_DATA_NAME, hr);
        return false;
    }

    // Step 2 — declare the same field layout as the sender
    hr = SimConnect_AddToClientDataDefinition(hSimConnect, DEF_ID,
        SIMCONNECT_CLIENTDATAOFFSET_AUTO, MESSAGE_SIZE);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to add client data definition: 0x{:08X}]\n", hr);
        return false;
    }

    // Step 3 — subscribe: deliver data to us whenever the sender calls SetClientData
    hr = SimConnect_RequestClientData(hSimConnect, CLIENT_DATA_ID, REQ_ID, DEF_ID,
        SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
        SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        0, 0, 0);
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to subscribe to client data: 0x{:08X}]\n", hr);
        return false;
    }

    std::cerr << "[Subscribed to client data area. Waiting for messages...]\n";
    return true;
}


/**
 * Run the message loop until the simulator shuts down or @p duration elapses.
 * Incoming CLIENT_DATA messages are printed to stdout.
 *
 * @param duration How long to wait for messages (0 = run until simulator quits).
 */
static void handleMessages(std::chrono::seconds duration)
{
    const bool haveNoDeadline{ duration.count() == 0 };
    if (haveNoDeadline) {
        std::cerr << "[Listening for messages until the simulator shuts down.]\n";
    }
    else {
        std::cerr << std::format("[Listening for messages for {} second(s).]\n", duration.count());
    }

    const auto endTime = std::chrono::steady_clock::now() + duration;

    while (haveNoDeadline || (std::chrono::steady_clock::now() <= endTime)) {
        const auto waitResult = ::WaitForSingleObject(hEvent, 100);
        if (waitResult == WAIT_TIMEOUT) {
            continue;
        }
        if (waitResult < 0) {
            std::cerr << std::format("[WaitForSingleObject failed: 0x{:08X}]\n", GetLastError());
            return;
        }

        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };

        while (SUCCEEDED(SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
                break;

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const auto* pOpen = toRecvPtr<SIMCONNECT_RECV_OPEN>(pData);
                std::cerr << std::format("[Connected to '{}' version {}.{} using SimConnect {}.{}]\n",
                    pOpen->szApplicationName,
                    pOpen->dwApplicationVersionMajor, pOpen->dwApplicationVersionMinor,
                    pOpen->dwSimConnectVersionMajor, pOpen->dwSimConnectVersionMinor);
            }
            break;

            case SIMCONNECT_RECV_ID_QUIT:
                std::cerr << "[Simulator is shutting down.]\n";
                return;

            case SIMCONNECT_RECV_ID_CLIENT_DATA:
            {
                // The data follows the SIMCONNECT_RECV_CLIENT_DATA header in memory
                const auto* pClientData = toRecvPtr<SIMCONNECT_RECV_CLIENT_DATA>(pData);
                if (pClientData->dwRequestID == REQ_ID) {
                    const auto* data = reinterpret_cast<const HelloWorldData*>(&pClientData->dwData);
                    std::cout << std::format("Received: '{}'\n", data->message);
                }
            }
            break;

            default:
                std::cerr << std::format("[Ignoring message of type {} ({} bytes)]\n",
                    pData->dwID, pData->dwSize);
                break;
            }
        }
    }
}


int main()
{
    if (!connect()) {
        return 1;
    }
    if (!subscribeClientData()) {
        disconnect();
        return 1;
    }

    handleMessages(60s);   // listen for up to 60 seconds
    disconnect();
    return 0;
}
