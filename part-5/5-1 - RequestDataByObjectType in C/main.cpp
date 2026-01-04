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

#include <format>
#include <iostream>


/**
 * A shorthand test if we need to avoid using MSFS 2024 specific features.
 */
#if defined(SIMCONNECT_TYPEDEF)
#define MSFS_2024_SDK 1     // NOLINT(cppcoreguidelines-macro-usage)
#else
#define MSFS_2024_SDK 0     // NOLINT(cppcoreguidelines-macro-usage)
#endif


constexpr DWORD reqId{ 1 };
constexpr DWORD aircraftInfoId{ 1 };

//
// From the SDK:
//
// "title": a string of max 128 characters
// "is user sim": a boolean
// "atc id": a string of max 10 characters
// "atc model": a string of max 10 characters
// "aircraft agl": a number (altitube "Above Ground Level")
// "plane altitude": a number
//

struct AircraftInfo {
    char     title[128];
    uint32_t isUserSim;
    char     atcId[32];
    char     atcModel[32];
    int32_t  altitudeAGL;
    int32_t  altitudeASL;
};

enum class DatumId : DWORD
{
    NoId = 0,
    Title,
    IsUser,
    AtcId,
    AtcModel,
    AltAGL,
    AltASL
};
consteval DWORD dword(DatumId v) { return static_cast<DWORD>(v); }

inline const uint8_t* toBytePtr(const void* ptr) { return reinterpret_cast<const uint8_t*>(ptr); }
inline const int32_t* toIntPtr(const void* ptr) { return reinterpret_cast<const int32_t*>(ptr); }
inline const DWORD* toDwordPtr(const void* ptr) { return reinterpret_cast<const DWORD*>(ptr); }
inline const char* toCharPtr(const void* ptr) { return reinterpret_cast<const char*>(ptr); }
inline const float* toFloatPtr(const void* ptr) { return reinterpret_cast<const float*>(ptr); }


template <typename Recv>
inline const Recv* toRecvPtr(const void* ptr) { return reinterpret_cast<const Recv*>(ptr); }

inline bool isTagged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_TAGGED) != 0); }
inline bool isChanged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_CHANGED) != 0); }

static HANDLE hSimConnect{ nullptr };		// The connection handle
static bool connected{ false };				// Do we have a live connection?


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
 * Dump the raw data to the console in hex, with an ASCII view next to it.
 *
 * @param data The data to dump.
 * @param dataSize The size of the data.
 */
static void hexDump(const uint8_t* data, unsigned dataSize) {
    printf("\n\nRaw data: (%d bytes)\n\n", dataSize);
    unsigned count{ 0 };

    while (count < dataSize) {
        if ((count % 16) == 0) {
            printf("0x%04x ", count);
        }
        printf(" 0x%02x", data[count]);
        count += 1;
        if ((count % 16) == 0) {
            printf("  ");
            for (unsigned p = count - 16; p < count; p++) {
                printf("%c", ((data[p] < 0x20) || (data[p] > 0x7f)) ? '.' : ((char)(data[p])));
            }
            printf("\n");
        }
    }
    if ((count % 16) != 0) {
        while ((count % 16) != 0) {
            printf("     ");
            count++;
        }
        printf(" ");
        for (unsigned p = count - 16; p < dataSize; p++) {
            printf("%c", ((data[p] < 0x20) || (data[p] > 0x7f)) ? '.' : ((char)(data[p])));
        }
        printf("\n\n");
    }
    else {
        printf("\n");
    }
}

/**
 * Parse the received SimObject Data if it uses the untagged format.
 *
 * @param ptr The pointer to the data.
 * @param dataSize The size of the data.
 */
[[maybe_unused]]
static void parseUntagged([[maybe_unused]] const uint8_t* ptr, [[maybe_unused]] unsigned dataSize, [[maybe_unused]] AircraftInfo& data)
{
    size_t i{ 0 };
    // First item: Title
    const char* title{ toCharPtr(&(ptr[i])) };
    printf("Aircraft title is '%s'.\n", title);
    i += 128;

    // Second item: isUser
    printf("This %s the user's aircraft.\n", (ptr[i] ? "IS" : "ISN'T"));
    i += sizeof(DWORD);

    // Third item: ATC Id
    const char* atcId{ toCharPtr(&(ptr[i])) };
    printf("Aircraft ATC Id is '%s'.\n", atcId);
    i += 32;

    // Fourth item: ATC Model
    const char* atcModelInMsg{ toCharPtr(&(ptr[i])) };
    char atcModel[32];
    strncpy_s(atcModel, atcModelInMsg, 32);
    printf("Aircraft ATC Model is '%s'.\n", atcModel);
    i += 32;

    // Fifth item: Aircraft altitude Above Ground Level
    int32_t aAGL = *toIntPtr(&(ptr[i]));
    printf("Aircraft is %d feet above ground level.\n", aAGL);
    i += sizeof(int32_t);

    // Sixt item: Altitude
    int32_t alt = *toIntPtr(&(ptr[i]));
    if (alt == 0)
    {
        printf("Aircraft is at sea level.\n");
    }
    else if (alt > 0)
    {
        printf("Aircraft is %d feet above sea level.\n", alt);
    }
    else
    {
        printf("Aircraft is %d feet below sea level.\n", -alt);
    }
    i += sizeof(int32_t);

    if (i < dataSize) {
        printf("Skipping %d unused byte(s).\n", int(dataSize - i));
    }
    else if (i > dataSize) {
        printf("Not enough data!\n");
    }
}

/**
 * Parse the received SimObject Data if it uses the tagged format.
 *
 * @param ptr The pointer to the data.
 * @param dataSize The size of the data.
 */
[[maybe_unused]]
static void parseTagged([[maybe_unused]] const uint8_t* ptr, [[maybe_unused]] unsigned dataSize, [[maybe_unused]] unsigned datumCount, [[maybe_unused]] AircraftInfo& data)
{
    size_t i = 0;
    while (i < dataSize)
    {
        DatumId id = *reinterpret_cast<const DatumId*>(&(ptr[i]));
        i += sizeof(DWORD);
        if (id == DatumId::NoId)
        {
            continue; // Skip
        }

        switch (id)
        {
        case DatumId::Title:
        {
            const char* title{ toCharPtr(&(ptr[i])) };
            printf("Aircraft title is '%s'.\n", title);
            i += strlen(title) + 1;
            if ((i % sizeof(DWORD)) != 0)
            {
                i += (4 - (i % sizeof(DWORD)));
            }
        }
        break;

        case DatumId::IsUser:
        {
            printf("This %s the user's aircraft.\n", (ptr[i] ? "IS" : "ISN'T"));
            i += sizeof(DWORD);
        }
        break;

        case DatumId::AtcId:
        {
            const char* atcId{ toCharPtr(&(ptr[i])) };
            printf("Aircraft ATC Id is '%s'.\n", atcId);
            i += strlen(atcId) + 1;
            if ((i % sizeof(DWORD)) != 0)
            {
                i += (4 - (i % sizeof(DWORD)));
            }
        }
        break;

        case DatumId::AtcModel:
        {
            const char* atcModelInMsg{ toCharPtr(&(ptr[i])) };
            char atcModel[32];
            strncpy_s(atcModel, atcModelInMsg, 32);
            printf("Aircraft ATC Model is '%s'.\n", atcModel);
            i += 32;
        }
        break;

        case DatumId::AltAGL:
        {
            int32_t aAGL = *toIntPtr(&(ptr[i]));
            printf("Aircraft is %d feet above ground level.\n", aAGL);
            i += sizeof(int32_t);
        }
        break;

        case DatumId::AltASL:
        {
            int32_t alt = *toIntPtr(&(ptr[i]));
            if (alt == 0)
            {
                printf("Aircraft is at sea level.\n");
            }
            else if (alt > 0)
            {
                printf("Aircraft is %d feet above sea level.\n", alt);
            }
            else
            {
                printf("Aircraft is %d feet below sea level.\n", -alt);
            }
            i += sizeof(int32_t);
        }
        break;
        }
    }
    if (i < dataSize) {
        printf("Skipping %d unused byte(s).\n", int(dataSize - i));
    }
    else if (i > dataSize) {
        printf("Not enough data!\n");
    }
}

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

        while (SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
            switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
                break;

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const SIMCONNECT_RECV_OPEN* pOpen = toRecvPtr<SIMCONNECT_RECV_OPEN>(pData);

                printf("Connected to '%s' version %d.%d (build %d.%d)\n",
                    pOpen->szApplicationName,
                    pOpen->dwApplicationVersionMajor,
                    pOpen->dwApplicationVersionMinor,
                    pOpen->dwApplicationBuildMajor,
                    pOpen->dwApplicationBuildMinor);
                printf("  using SimConnect version %d.%d (build %d.%d)\n",
                    pOpen->dwSimConnectVersionMajor,
                    pOpen->dwSimConnectVersionMinor,
                    pOpen->dwSimConnectBuildMajor,
                    pOpen->dwSimConnectBuildMinor);
            }
            break;

            case SIMCONNECT_RECV_ID_QUIT:
            {
                printf("Simulator is shutting down.\n");
                connected = false;
            }
            break;

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
			case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
            {
                const SIMCONNECT_RECV_SIMOBJECT_DATA* msg = toRecvPtr<SIMCONNECT_RECV_SIMOBJECT_DATA>(pData);

                if (msg->dwRequestID != reqId) {
                    printf("Ignoring data for request %d. (this isn't ours)\n", msg->dwRequestID);
                }
                else if (msg->dwDefineID != aircraftInfoId) {
                    printf("Ignoring data for Define ID %d. (this isn't ours)\n", msg->dwDefineID);
                }
                else {
                    unsigned dataSize{ cbData - (4 * 10) };
                    printf("Received SimObject data for request %d, object %d, defineId %d, %d items, entry %d out of %d, remaining message size %d bytes.\n",
                        msg->dwRequestID, msg->dwObjectID, msg->dwDefineID, msg->dwDefineCount,
                        msg->dwentrynumber, msg->dwoutof,
                        dataSize);
                    if (isChanged(msg)) {
                        printf("  - Data is sent due to a change.\n");
                    }
                    if (isTagged(msg)) {
                        printf("  - Data is in the TAGGED format.\n");
                    }

                    hexDump(toBytePtr(&(msg->dwData)), dataSize);

                    AircraftInfo data;
                    if (isTagged(msg)) {
                        parseTagged(toBytePtr(&(msg->dwData)), dataSize, msg->dwDefineCount, data);
                    }
                    else {
                        parseUntagged(toBytePtr(&(msg->dwData)), dataSize, data);
                    }
                }

            }
            break;

            default:
                printf("Ignoring message of type %d (length %d bytes)\n", pData->dwID, pData->dwSize);
                break;
            }
        }
        if (connected) {
            Sleep(100);
        }
    }
}

/**
 * Run some tests.
 *
 * @return Zero if ok, non-zero if not.
 */
static int testConnect()
{
    HANDLE hEventHandle{ ::CreateEvent(NULL, FALSE, FALSE, NULL) };
    if (hEventHandle == NULL) {
        printf("Failed to create a Windows Event!\n");
        return 1;
    }

    HRESULT hr = SimConnect_Open(&hSimConnect, "My First SimConnect App", nullptr, 0, hEventHandle, 0);

    if (SUCCEEDED(hr)) {
        std::cout << "Successfully connected to MSFS.\n";

        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "title", nullptr, SIMCONNECT_DATATYPE_STRING128, 0, int(DatumId::Title));
        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "is user sim", "bool", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::IsUser));
        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "atc id", nullptr, SIMCONNECT_DATATYPE_STRING32, 0, int(DatumId::AtcId));
        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "atc model", nullptr, SIMCONNECT_DATATYPE_STRING32, 0, int(DatumId::AtcModel));
        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "aircraft AGL", "feet", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::AltAGL));
        SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "plane altitude", "feet", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::AltASL));

        SimConnect_RequestDataOnSimObjectType(hSimConnect, reqId, aircraftInfoId, 1000, SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);

        connected = true;
        handle_messages(hEventHandle);

        hr = SimConnect_Close(hSimConnect);

        std::cout << "Disconnected from MSFS.\n";
    }
    else {
        std::cerr << "Failed to connect to MSFS!\n";
    }
    return FAILED(hr); // Zero is "ok"
}


int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    std::cout << "Welcome to my first SimConnect app.\n";

    return testConnect();
}