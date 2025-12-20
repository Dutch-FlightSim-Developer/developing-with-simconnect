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

// The other headers need this one, but don't include it themselves...
#include <windows.h> // NOLINT(misc-include-cleaner)

#include <minwindef.h>
#include <WinBase.h>
#include <winnt.h>
#include <synchapi.h>
#include <winerror.h>

// Stupid strncpy vs strncpy_s Catch-22
#include <string.h> // NOLINT(hicpp-deprecated-headers,modernize-deprecated-headers)

#include "SimConnect.h"

#pragma warning(pop)


#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <array>
#include <format>
#include <string_view>
#include <iostream>


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

inline constexpr size_t titleSize{ 128 };
inline constexpr size_t atcIdSize{ 32 };
inline constexpr size_t atcModelSize{ 32 };

struct AircraftInfo {
	std::array<char, titleSize>     title;
	uint32_t                        isUserSim;
	std::array<char, atcIdSize>     atcId;
	std::array<char, atcModelSize>  atcModel;
	int32_t                         altitudeAGL;
	int32_t                         altitudeASL;
};

enum class DatumId : std::uint8_t
{
    NoId = 0,
    Title,
    IsUser,
    AtcId,
    AtcModel,
	AltAGL,
	AltASL
};
consteval DWORD dword(DatumId value) { return static_cast<DWORD>(value); }

inline const uint8_t *toBytePtr(const void *ptr) { return reinterpret_cast<const uint8_t *>(ptr); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
inline const int32_t *toIntPtr(const void *ptr) { return reinterpret_cast<const int32_t *>(ptr); }  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
inline const DWORD *toDwordPtr(const void *ptr) { return reinterpret_cast<const DWORD *>(ptr); }    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
inline const char *toCharPtr(const void *ptr) { return reinterpret_cast<const char *>(ptr); }       // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
inline const float *toFloatPtr(const void *ptr) { return reinterpret_cast<const float *>(ptr); }    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

template <typename Recv>
inline const Recv* toRecvPtr(const void *ptr) { return reinterpret_cast<const Recv *>(ptr); }       // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

inline bool isTagged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_TAGGED) != 0); }
inline bool isChanged(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg) { return ((msg->dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_CHANGED) != 0); }


/**
 * Handle SimConnect Exception messages.ABC
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
    case SIMCONNECT_EXCEPTION_INTERNAL:
        break;
        // No default; we want an error if we miss one
    }
}


inline static constexpr unsigned bytesPerLine{ 16 };
inline static constexpr char firstPrintableChar{ 0x20 };
inline static constexpr char lastPrintableChar{ 0x7f };

/**
 * Dump the raw data to the console in hex, with an ASCII view next to it.
 *
 * @param data The data to dump.
 * @param dataSize The size of the data.
 */
static void hexDump(const uint8_t* data, unsigned dataSize) {
	std::cout << std::format("\n\nRaw data: ({} bytes)\n\n", dataSize);
	unsigned count{ 0 };

    while (count < dataSize) {
		if ((count % bytesPerLine) == 0) {
            std::cout << std::format("0x{:04x} ", count);
        }
        std::cout << std::format(" 0x{:02x}", static_cast<int>(data[count])); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        count += 1;
		if ((count % bytesPerLine) == 0) {
            std::cout << "  ";
			for (unsigned printable = count - bytesPerLine; printable < count; printable++) {
                const unsigned char printableChar = data[printable]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::cout << std::format("{}", ((printableChar < firstPrintableChar) || (printableChar > lastPrintableChar)) ? '.' : printableChar);
            }
            std::cout << "\n";
        }
    }
	if ((count % bytesPerLine) != 0) {
		while ((count % bytesPerLine) != 0) {
            std::cout << "     ";
            count++;
        }
        std::cout << " ";
		for (unsigned printable = count - bytesPerLine; printable < dataSize; printable++) {
                const unsigned char printableChar = data[printable]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::cout << std::format("{}", ((printableChar < firstPrintableChar) || (printableChar > lastPrintableChar)) ? '.' : printableChar);
        }
        std::cout << "\n\n";
    }
	else {
        std::cout << "\n";
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
    size_t index{0};
    // First item: Title
    const char *title{ toCharPtr(&(ptr[index])) }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cout << std::format("Aircraft title is '{}'.\n", title);
    index += strlen(title) + 1;
    if ((index % sizeof(DWORD)) != 0)
    {
        index += (4 - (index % sizeof(DWORD)));
    }

    // Second item: isUser
    std::cout << std::format("This {} the user's aircraft.\n", ((ptr[index] != 0) ? "IS" : "ISN'T")); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    index += sizeof(DWORD);

    // Third item: ATC Id
    const char *atcId{toCharPtr(&(ptr[index]))}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cout << std::format("Aircraft ATC Id is '{}'.\n", atcId);
    index += strlen(atcId) + 1;
    if ((index % sizeof(DWORD)) != 0)
    {
        index += (4 - (index % sizeof(DWORD)));
    }

    // Fourth item: ATC Model
    const char *atcModelInMsg{toCharPtr(&(ptr[index]))}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::array<char, atcModelSize> atcModel{ 0 };
    strncpy_s(atcModel.data(), atcModel.size(), atcModelInMsg, atcModelSize);
    std::cout << std::format("Aircraft ATC Model is '{}'.\n", atcModel.data());
    index += atcModelSize;

    // Fifth item: Aircraft altitude Above Ground Level
    int32_t aAGL = *toIntPtr(&(ptr[index])); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cout << std::format("Aircraft is {} feet above ground level.\n", aAGL);
    index += sizeof(int32_t);

    // Sixt item: Altitude
    int32_t alt = *toIntPtr(&(ptr[index])); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (alt == 0)
    {
        std::cout << "Aircraft is at sea level.\n";
    }
    else if (alt > 0)
    {
        std::cout << std::format("Aircraft is {} feet above sea level.\n", alt);
    }
    else
    {
        std::cout << std::format("Aircraft is {} feet below sea level.\n", -alt);
    }
    index += sizeof(int32_t);

	if (index < dataSize) {
        std::cerr << std::format("Skipping {} unused byte(s).\n", int(dataSize - index));
    }
	else if (index > dataSize) {
        std::cerr << "Not enough data!\n";
    }
}

/**
 * Parse the received SimObject Data if it uses the tagged format.
 *
 * @param ptr The pointer to the data.
 * @param dataSize The size of the data.
 */
[[maybe_unused]]
static void parseTagged([[maybe_unused]] const uint8_t* ptr, [[maybe_unused]] unsigned dataSize, [[maybe_unused]] AircraftInfo& data, [[maybe_unused]] unsigned datumCount)
{
    size_t index = 0;
    while (index < dataSize)
    {
        const DatumId datumId = *reinterpret_cast<const DatumId *>(&(ptr[index])); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
        index += sizeof(DWORD);
        if (datumId == DatumId::NoId)
        {
            continue; // Skip
        }

        switch (datumId)
        {
        case DatumId::NoId:
            //IGNORE
        break;

        case DatumId::Title:
        {
            const char *title{toCharPtr(&(ptr[index]))}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::cout << std::format("Aircraft title is '{}'.\n", title);
            index += strlen(title) + 1;
            if ((index % sizeof(DWORD)) != 0)
            {
                index += (4 - (index % sizeof(DWORD)));
            }
        }
        break;

        case DatumId::IsUser:
        {
            std::cout << std::format("This {} the user's aircraft.\n", ((ptr[index] != 0) ? "IS" : "ISN'T")); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            index += sizeof(DWORD);
        }
        break;

        case DatumId::AtcId:
        {
            const char *atcId{toCharPtr(&(ptr[index]))}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::cout << std::format("Aircraft ATC Id is '{}'.\n", atcId);
            index += strlen(atcId) + 1;
            if ((index % sizeof(DWORD)) != 0)
            {
                index += (4 - (index % sizeof(DWORD)));
            }
        }
        break;

        case DatumId::AtcModel:
        {
            const char *atcModelInMsg{toCharPtr(&(ptr[index]))}; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::array<char, atcModelSize> atcModel{ 0 };
            strncpy_s(atcModel.data(), atcModel.size(), atcModelInMsg, atcModelSize);
            std::cout << std::format("Aircraft ATC Model is '{}'.\n", std::string_view(atcModel.data(), atcModel.size()));
            index += atcModelSize;
        }
        break;

        case DatumId::AltAGL:
        {
            int32_t aAGL = *toIntPtr(&(ptr[index])); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::cout << std::format("Aircraft is {} feet above ground level.\n", aAGL);
            index += sizeof(int32_t);
        }
        break;

        case DatumId::AltASL:
        {
            int32_t alt = *toIntPtr(&(ptr[index])); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (alt == 0)
            {
                std::cout << "Aircraft is at sea level.\n";
            }
            else if (alt > 0)
            {
                std::cout << std::format("Aircraft is {} feet above sea level.\n", alt);
            }
            else
            {
                std::cout << std::format("Aircraft is {} feet below sea level.\n", -alt);
            }
            index += sizeof(int32_t);
        }
        break;
        }
    }
	if (index < dataSize) {
        std::cout << std::format("Skipping {} unused byte(s).\n", int(dataSize - index));
    }
	else if (index > dataSize) {
        std::cerr << "Not enough data!\n";
    }
}

/**
 * Handle SimObject data messages.
 *
 * @param msg The SimObject data message.
 * @param cbData The size of the complete message.
 */
static void handleSimObjectData(const SIMCONNECT_RECV_SIMOBJECT_DATA* msg, DWORD cbData)
{
    if (msg->dwRequestID != reqId) {
        std::cout << std::format("Ignoring data for request {}. (this isn't ours)\n", msg->dwRequestID);
    }
    else if (msg->dwDefineID != aircraftInfoId) {
        std::cout << std::format("Ignoring data for Define ID {}. (this isn't ours)\n", msg->dwDefineID);
    }
    else {
        constexpr unsigned nrFields{ 7 }; // 7 DWORDS in the header before the data
        unsigned dataSize{ static_cast<unsigned>(cbData - (sizeof(DWORD) * nrFields)) };
        std::cout << std::format("Received SimObject data for request {}, object {}, defineId {}, {} items, entry {} out of {}, remaining message size {} bytes.\n",
               msg->dwRequestID, msg->dwObjectID, msg->dwDefineID, msg->dwDefineCount,
               msg->dwentrynumber, msg->dwoutof,
               dataSize);
        if (isChanged(msg)) {
            std::cout << "  - Data is sent due to a change.\n";
        }
        if (isTagged(msg)) {
            std::cout << "  - Data is in the TAGGED format.\n";
        }

        hexDump(toBytePtr(&(msg->dwData)), dataSize);

        AircraftInfo data{};
        if (isTagged(msg)) {
            parseTagged(toBytePtr(&(msg->dwData)), dataSize, data, msg->dwDefineCount);
        }
        else {
            parseUntagged(toBytePtr(&(msg->dwData)), dataSize, data);
        }
        std::cout << std::format("Title: '{}'\n", std::string_view(data.title.data(), data.title.size()));
        std::cout << std::format("This is {}the user's aircraft.\n", (data.isUserSim != 0) ? "" : "NOT ");
        std::cout << std::format("ATC ID: '{}'\n", std::string_view(data.atcId.data(), data.atcId.size()));
        std::cout << std::format("ATC Model: '{}'\n", std::string_view(data.atcModel.data(), data.atcModel.size()));
        std::cout << std::format("The aicraft is {} feet AGL.\n", data.altitudeAGL);
        std::cout << std::format("The aircraft is {} feet {} sea level.\n", abs(data.altitudeASL), (data.altitudeASL >= 0) ? "above" : "below");
    }
}

/**
 * Handle messages from SimConnect.
 *
 * @param hEvent The event handle to wait for.
 */
static void handle_messages(HANDLE hEvent, HANDLE hSimConnect) // NOLINT(bugprone-easily-swappable-parameters)
{
    bool connected{ true };
	while (connected && (::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)) {
		SIMCONNECT_RECV* pData{ nullptr };
		DWORD cbData{ 0 };
		HRESULT result{ S_OK };

		while (SUCCEEDED(result = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
			switch (pData->dwID) {
            case SIMCONNECT_RECV_ID_EXCEPTION:
                handleException(*toRecvPtr<SIMCONNECT_RECV_EXCEPTION>(pData));
                break;

            case SIMCONNECT_RECV_ID_OPEN:
            {
                const auto *pOpen = toRecvPtr<SIMCONNECT_RECV_OPEN>(pData);

                std::cout << std::format("Connected to '{}' version {}.{} (build {}.{})\n",
                       pOpen->szApplicationName,
                       pOpen->dwApplicationVersionMajor,
                       pOpen->dwApplicationVersionMinor,
                       pOpen->dwApplicationBuildMajor,
                       pOpen->dwApplicationBuildMinor);
                std::cout << std::format("  using SimConnect version {}.{} (build {}.{})\n",
                       pOpen->dwSimConnectVersionMajor,
                       pOpen->dwSimConnectVersionMinor,
                       pOpen->dwSimConnectBuildMajor,
                       pOpen->dwSimConnectBuildMinor);
            }
            break;

            case SIMCONNECT_RECV_ID_QUIT:
            {
                std::cout << "Simulator is shutting down.\n";
                connected = false;
            }
            break;

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
            {
                const auto *msg = toRecvPtr<SIMCONNECT_RECV_SIMOBJECT_DATA>(pData);
                handleSimObjectData(msg, cbData);
            }
            break;

            default:
                std::cout << std::format("Ignoring message of type {} (length {} bytes)\n", pData->dwID, pData->dwSize);
                break;
            }
        }
		if (connected) {
            constexpr DWORD waitMs{ 100 };
            Sleep(waitMs);
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
    HANDLE hEventHandle{ ::CreateEvent(nullptr, FALSE, FALSE, nullptr) };
	if (hEventHandle == nullptr) {
        std::cerr << "Failed to create a Windows Event!\n";
        return 1;
    }

    HANDLE hSimConnect{ nullptr };

    HRESULT result = SimConnect_Open(&hSimConnect, "My First SimConnect App", nullptr, 0, hEventHandle, 0);

	if (SUCCEEDED(result)) {
        std::cout << "Successfully connected to MSFS.\n";

		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "title", nullptr, SIMCONNECT_DATATYPE_STRINGV, 0, int(DatumId::Title));
		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "is user sim", "bool", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::IsUser));
		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "atc id", nullptr, SIMCONNECT_DATATYPE_STRINGV, 0, int(DatumId::AtcId));
		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "atc model", nullptr, SIMCONNECT_DATATYPE_STRINGV, 0, int(DatumId::AtcModel));
		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "aircraft AGL", "feet", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::AltAGL));
		SimConnect_AddToDataDefinition(hSimConnect, aircraftInfoId, "plane altitude", "feet", SIMCONNECT_DATATYPE_INT32, 0, int(DatumId::AltASL));

		SimConnect_RequestDataOnSimObject(hSimConnect, reqId, aircraftInfoId, SIMCONNECT_SIMOBJECT_TYPE_USER_AIRCRAFT, SIMCONNECT_PERIOD_ONCE, SIMCONNECT_DATA_REQUEST_FLAG_TAGGED);

        handle_messages(hEventHandle, hSimConnect);

        result = SimConnect_Close(hSimConnect);

        std::cout << "Disconnected from MSFS.\n";
    }
	else {
        std::cerr << "Failed to connect to MSFS!\n";
    }
    return FAILED(result); // Zero is "ok"
}


auto main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    std::cout << "Welcome to my first SimConnect app.\n";

    return testConnect();
}