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
#include <simconnect/requests/system_state_handler.hpp>

#include <format>
#include <iostream>


using namespace std::chrono_literals;

inline static std::string version(int major, int minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}

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

static void handleOpen(const SIMCONNECT_RECV_OPEN& msg) {
	std::cout << "Connected to " << msg.szApplicationName
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << std::endl
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << std::endl
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << std::endl
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << std::endl;
}

static void handleClose([[maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
}


auto main() -> int {
	SimConnect::WindowsEventConnection connection;
	SimConnect::WindowsEventHandler handler(connection);
	handler.autoClosing(true);

	handler.registerDefaultHandler([](const SIMCONNECT_RECV& msg) {
		std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
		});
	handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, handleException);
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen);
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose);

	std::cout << "Opening connection\n";
	if (connection.open()) {
		SimConnect::SystemStateHandler<SimConnect::WindowsEventHandler<>> requestHandler(handler);
		requestHandler.enable(handler);

		requestHandler.requestSystemState("AircraftLoaded",
			[](std::string aircraft) {
				std::cout << std::format("Currently loaded aircraft '{}'.\n", aircraft);
			});

		requestHandler.requestSystemState("DialogMode",
			[](bool inDialog) {
				std::cout << (inDialog ? "The user is now in a dialog.\n" : "The user is now NOT in a dialog.\n");
			});

		requestHandler.requestSystemState("FlightLoaded",
			[](std::string flight) {
				std::cout << std::format("Currently loaded flight '{}'.\n", flight);
			});

		requestHandler.requestSystemState("FlightPlan",
			[](std::string flightPlan) {
				std::cout << std::format("Currently loaded flightplan '{}'.\n", flightPlan);
			});

		requestHandler.requestSystemState("Sim",
			[](bool flying) {
				std::cout << (flying ? "The user is now in control of the aircraft.\n" : "The user is now navigating the UI.\n");
			});

		requestHandler.requestSystemState("SimLoaded",
			[](std::string simulator) {
				std::cout << std::format("Currently loaded simulator '{}'.\n", simulator);
			}); // Will cause an exception message

		std::cout << "Handling messages\n";
		handler.handle(10s);
	}
	else {
		std::cerr << "Failed to open connection\n";
	}
	return 0;
}