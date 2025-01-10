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

#include <format>
#include <iostream>


using namespace std::chrono_literals;


/**
 * Produce a nicely formatted version string.
 *
 * @param major the major version number. If `0`, the version is considered to be unknown.
 * @param minor the minor version number. If `0`, the minor version number is not appended.
 * @return the formatted version string.
 */
inline static std::string version(int major, int minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


/**
 * Handle a `SIMCONNECT_RECV_OPEN` message.
 *
 * @param msg the message.
 */
static void handleOpen(const SIMCONNECT_RECV_OPEN& msg) {
	std::cout << "Connected to " << msg.szApplicationName
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << std::endl
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << std::endl
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << std::endl
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << std::endl;
}


/**
 * Handle a `SIMCONNECT_RECV_QUIT` message.
 *
 * @param msg the message.
 */
static void handleClose(const SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
}


/**
 * Request a system state.
 *
 * @param connection the simulator connection to use.
 * @param stateName the name of the state to request.
 */
static void requestSystemState(SimConnect::Connection& connection, const char* stateName) {
	int reqId{ connection.requestSystemState(stateName) };
	if (connection) {
		std::cout << std::format("Requested system state '{}' using request ID {}\n", stateName, reqId);
	}
	else {
		std::cerr << std::format("Failed to request system state '{}': {:0x}\n", stateName, connection.lastResult());
	}
}


/**
 * Handle a `SIMCONNECT_RECV_SYSTEM_STATE` message.
 *
 * @param msg the message.
 */
static void handleSystemState(const SIMCONNECT_RECV_SYSTEM_STATE& msg) {
	std::cout << std::format("Received system state for request {}: {}, {}, '{}'\n", (int)msg.dwRequestID, msg.dwInteger, msg.fFloat, msg.szString);
}


/** Main entry point. */
auto main () -> int {
	SimConnect::WindowsEventConnection connection;				// Use a Windows Event.
	SimConnect::WindowsEventHandler handler(connection);		// Use a Windows Event.
	handler.autoClosing(true);

	handler.setDefaultHandler([](const SIMCONNECT_RECV* msg, DWORD len) {
			std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg->dwID, len);
		});
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen);
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose);
	handler.registerHandler<SIMCONNECT_RECV_SYSTEM_STATE>(SIMCONNECT_RECV_ID_SYSTEM_STATE, handleSystemState);

	if (connection.open()) {
		requestSystemState(connection, "AircraftLoaded");
		requestSystemState(connection, "DialogMode");
		requestSystemState(connection, "FlightLoaded");
		requestSystemState(connection, "FlightPlan");
		requestSystemState(connection, "Sim");
		requestSystemState(connection, "SimLoaded"); // Will cause an exception message

		std::cout << "Handling messages\n";
		handler.handle(30s);
	}
	return 0;
}