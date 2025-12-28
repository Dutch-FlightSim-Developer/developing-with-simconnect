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


constexpr static const char* appName = "SimConnect Console Application";
static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };



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
 * Handle messages from SimConnect.
 */
static void handle_messages()
{
    bool connected{ true };
    while (connected && (::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)) {
        SIMCONNECT_RECV* pData{ nullptr };
        DWORD cbData{ 0 };
        HRESULT hr{ S_OK };

        int count{ 0 };

        while (SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
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

            case SIMCONNECT_RECV_ID_AIRPORT_LIST:
            {
                auto pAirportList = toRecvPtr<SIMCONNECT_RECV_AIRPORT_LIST>(pData);

                count += pAirportList->dwArraySize;

                // Print progress
                std::cerr << std::format("[Received {} airport messages (part {} of {}), total received: {}]\n",
                    pAirportList->dwArraySize,
                    pAirportList->dwEntryNumber + 1,
                    pAirportList->dwOutOf,
                    count);

                for (DWORD i = 0; i < pAirportList->dwArraySize; ++i) {
                    const auto& airportData = pAirportList->rgData[i];
                    std::cout << std::format("Airport ICAO: '{}', Region: '{}'\n", airportData.Ident, airportData.Region);
                }


                // If this was the last part, exit the message loop
                if ((pAirportList->dwEntryNumber+1) >= pAirportList->dwOutOf) {
                    connected = false; // Exit the message loop
                }
            }
            break;

            case SIMCONNECT_RECV_ID_WAYPOINT_LIST:
            {
                auto pWaypointList = toRecvPtr<SIMCONNECT_RECV_WAYPOINT_LIST>(pData);

                count += pWaypointList->dwArraySize;

                // Print progress
                std::cerr << std::format("[Received {} Waypoint messages (part {} of {}), total received: {}]\n",
                    pWaypointList->dwArraySize,
                    pWaypointList->dwEntryNumber + 1,
                    pWaypointList->dwOutOf,
                    count);

                for (DWORD i = 0; i < pWaypointList->dwArraySize; ++i) {
                    const auto& waypointData = pWaypointList->rgData[i];
                    std::cout << std::format("Waypoint ID: '{}', Region: '{}', LatLonAlt: {}/{}/{}, Magnetic variation: {}\n",
                        waypointData.Ident, waypointData.Region,
                        waypointData.Latitude, waypointData.Longitude, waypointData.Altitude, waypointData.fMagVar);
                }


                // If this was the last part, exit the message loop
                if ((pWaypointList->dwEntryNumber + 1) >= pWaypointList->dwOutOf) {
                    connected = false; // Exit the message loop
                }
            }
            break;

            case SIMCONNECT_RECV_ID_NDB_LIST:
            {
                auto pNDBList = toRecvPtr<SIMCONNECT_RECV_NDB_LIST>(pData);

                count += pNDBList->dwArraySize;

                // Print progress
                std::cerr << std::format("[Received {} NDB messages (part {} of {}), total received: {}]\n",
                    pNDBList->dwArraySize,
                    pNDBList->dwEntryNumber + 1,
                    pNDBList->dwOutOf,
                    count);

                for (DWORD i = 0; i < pNDBList->dwArraySize; ++i) {
                    const auto& ndbData = pNDBList->rgData[i];
                    std::cout << std::format("NDB ID: '{}', Region: '{}', Frequency: {:05.1f} kHz, LatLonAlt: {:.2f}/{:.2f}/{:.2f}m, Magnetic variation: {}\n",
                        ndbData.Ident, ndbData.Region, ndbData.fFrequency / 1000.0,
                        ndbData.Latitude, ndbData.Longitude, ndbData.Altitude, ndbData.fMagVar);
                }


                // If this was the last part, exit the message loop
                if ((pNDBList->dwEntryNumber + 1) >= pNDBList->dwOutOf) {
                    connected = false; // Exit the message loop
                }
            }
            break;

            case SIMCONNECT_RECV_ID_VOR_LIST:
            {
                auto pVORList = toRecvPtr<SIMCONNECT_RECV_VOR_LIST>(pData);

                count += pVORList->dwArraySize;

                // Print progress
                std::cerr << std::format("[Received {} VOR messages (part {} of {}), total received: {}]\n",
                    pVORList->dwArraySize,
                    pVORList->dwEntryNumber + 1,
                    pVORList->dwOutOf,
                    count);

                for (DWORD i = 0; i < pVORList->dwArraySize; ++i) {
                    const auto& vorData = pVORList->rgData[i];
                    std::cout << std::format("VOR ID: '{}', Region: '{}', Frequency: {:06.2f} MHz, LatLonAlt: {:.2f}/{:.2f}/{:.2f}m, Magnetic variation: {}\n",
                        vorData.Ident, vorData.Region, vorData.fFrequency / 1'000'000.0,
                        vorData.Latitude, vorData.Longitude, vorData.Altitude, vorData.fMagVar);
                }


                // If this was the last part, exit the message loop
                if ((pVORList->dwEntryNumber + 1) >= pVORList->dwOutOf) {
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


constexpr static const SIMCONNECT_DATA_REQUEST_ID REQUEST_ID{ 1 };


enum class QueryType {
    All,
    Bubble,
    Cache
};


auto main(int argc, const char* argv[]) -> int
{
    SIMCONNECT_FACILITY_LIST_TYPE objType = SIMCONNECT_FACILITY_LIST_TYPE_VOR;
	QueryType queryType{ QueryType::Cache };

	int nextArg{ 1 };
    if (argc > nextArg) {
        std::string arg{ argv [nextArg] };
        if (arg == "airport") {
            objType = SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT;
            nextArg++;
        }
        else if (arg == "waypoint") {
            objType = SIMCONNECT_FACILITY_LIST_TYPE_WAYPOINT;
            nextArg++;
        }
        else if (arg == "ndb") {
            objType = SIMCONNECT_FACILITY_LIST_TYPE_NDB;
            nextArg++;
        }
        else if (arg == "vor") {
            objType = SIMCONNECT_FACILITY_LIST_TYPE_VOR;
            nextArg++;
        }
    }
    if (argc > nextArg) {
		std::string option{ argv [nextArg] };
        if (option == "--all") {
			queryType = QueryType::All;
			nextArg++;
        }
        else if (option == "--bubble") {
            queryType = QueryType::Bubble;
            nextArg++;
        }
		else if (option == "--cache") {
			queryType = QueryType::Cache;
			nextArg++;
        }
    }
    if (argc > nextArg) {
        std::cerr << "Invalid number of arguments.\n"
            "Usage: " << argv[0] << " [airport|waypoint|ndb|vor] [--all|--bubble|--cache]\n";
        return -1;
    }

    if (!connect()) {
		std::cerr << "Unable to connect to MSFS 2024.\n";
        return -1;
    }
    std::cerr << "[Connected to MSFS 2024]\n";

    HRESULT hr{ 0 };
    switch (queryType) {
    case QueryType::All:
        hr = SimConnect_RequestAllFacilities(hSimConnect, objType, REQUEST_ID);
		break;
    case QueryType::Bubble:
        hr = SimConnect_RequestFacilitiesList_EX1(hSimConnect, objType, REQUEST_ID);
        break;
    case QueryType::Cache:
        hr = SimConnect_RequestFacilitiesList(hSimConnect, objType, REQUEST_ID);
		break;
    }
    if (FAILED(hr)) {
        std::cerr << std::format("[Failed to request facilities list: 0x{:08X}]\n", hr);

        disconnect();
        return -1;
    }
    handle_messages();

    disconnect();

    return 0;
}