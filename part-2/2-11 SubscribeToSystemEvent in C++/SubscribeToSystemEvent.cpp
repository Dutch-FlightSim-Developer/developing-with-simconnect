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


#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/events/system_events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/util/logger.hpp>
#include <simconnect/util/console_logger.hpp>

#include <chrono>
#include <string>
#include <format>
#include <iostream>


using namespace std::chrono_literals;


/**
 * Return a formatted string of the version. if the major number is 0, it returns "Unknown". The lower number is ignored if 0.
 * 
 * @param major The major version number.
 * @param minor The minor version number.
 * @returns a string with the formatted version number.
 */
inline static std::string version(unsigned long major, unsigned long minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


// Lots of references to constants defined in SimConnect.h
// NOLINTBEGIN(misc-include-cleaner)

/**
 * Handle an exception message, printing details to the standard error output.
 * 
 * @param msg The exception message received.
 */
static void handleException(const SIMCONNECT_RECV_EXCEPTION& msg) {
	const SIMCONNECT_EXCEPTION exc{ static_cast<SIMCONNECT_EXCEPTION>(msg.dwException) };
	std::cerr << std::format("Received an exception type {}:\n", (int)exc);
	if (msg.dwSendID != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_SENDID) {
		std::cerr << std::format("- Related to a message with SendID {}.\n", (int)msg.dwSendID);
	}
	if (msg.dwIndex != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_INDEX) {
		std:: cerr << std::format("- Regarding parameter {}.\n", (int)msg.dwIndex);
	}
	switch (exc)
	{
	case SIMCONNECT_EXCEPTION_NONE:										// Should never happen
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
	case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:						// Legacy
		std::cerr << "The weather port is invalid.\n";
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:					// Legacy
		std::cerr << "The METAR string is invalid.\n";
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:		// Legacy
		std::cerr << "Unable to get the observation.\n";
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:			// Legacy
		std::cerr << "Unable to create the station.\n";
		break;
	case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:			// Legacy
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
        std::cerr << "An internal error has occurred.\n";
		break;
		// No default; we want an error if we miss one
	}
}

// NOLINTEND(misc-include-cleaner)

/**
 * Print the information of the "Open" message, which tells us some details about the simulator.
 * 
 * @param msg The message received.
 */
static void handleOpen(const SIMCONNECT_RECV_OPEN& msg) { // NOLINT(misc-include-cleaner)
	std::cout << "Connected to " << &msg.szApplicationName[0]
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << '\n'
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << '\n'
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << '\n'
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << '\n';
}


/**
 * Tell the user the simulator is shutting down.
 * 
 * @param msg The message received.
 */
static void handleClose([[maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) { // NOLINT(misc-include-cleaner)
	std::cout << "Simulator shutting down.\n";
}


/**
 * Handle the SIMCONNECT_RECV_EVENT message.
 */
static void handleEvent(const SIMCONNECT_RECV_EVENT& msg) { // NOLINT(misc-include-cleaner)
	if (msg.uGroupID == SIMCONNECT_RECV_EVENT::UNKNOWN_GROUP) {
		std::cout << std::format("Received event '{}' ({}): dwData = {}\n",
			SimConnect::event::get(msg.uEventID).name(), msg.uEventID, msg.dwData);
	}
	else {
		std::cout << std::format("Received event '{}' ({}) in group {}: dwData = {}\n",
			SimConnect::event::get(msg.uEventID).name(), msg.uEventID, msg.uGroupID, msg.dwData);
	}
}


using MyConnection = SimConnect::WindowsEventConnection<false, SimConnect::ConsoleLogger>;
using MyMessageHandler = SimConnect::WindowsEventHandler<false, SimConnect::ConsoleLogger>;


/**
 * Demonstrate how to subscribe to system events.
 */
auto main() -> int { // NOLINT(bugprone-exception-escape)
	MyConnection connection;
	MyMessageHandler handler(connection, SimConnect::LogLevel::Debug);
	handler.autoClosing(true);

	handler.registerDefaultHandler([](const SIMCONNECT_RECV& msg) { // NOLINT(misc-include-cleaner)
		std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
	});
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen); // NOLINT(misc-include-cleaner)
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose); // NOLINT(misc-include-cleaner)
    handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, handleException); // NOLINT(misc-include-cleaner)
	handler.registerHandler<SIMCONNECT_RECV_EVENT>(SIMCONNECT_RECV_ID_EVENT, handleEvent); // NOLINT(misc-include-cleaner)

	if (connection.open()) {
		SimConnect::EventHandler<MyMessageHandler> eventHandler(handler);
		SimConnect::SystemEvents<MyMessageHandler> systemEvents(eventHandler);

		systemEvents.subscribeToSystemEvent(SimConnect::Events::sim(), [](const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'Sim' event with value {}.\n", msg.dwData);
		});
		systemEvents.subscribeToSystemEvent(SimConnect::Events::simStart(), []([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'SimStart' event.\n");
		});
		systemEvents.subscribeToSystemEvent(SimConnect::Events::simStop(), []([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'SimStop' event.\n");
        });
		systemEvents.subscribeToSystemEvent(SimConnect::Events::pause(), [](const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'Pause' event with value {}.\n", msg.dwData);
        });

		std::cout << "\n\nHandling messages for 30 seconds.\n";
        constexpr auto duration = 30s;
		handler.handle(duration);
	}
	else {
		std::cerr << "Failed to connect to simulator.\n";
	}

	return 0;
}