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
#include <simconnect/request_handler.hpp>

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
static void handleClose([[ maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
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

	if (connection.open()) {
		SimConnect::RequestHandler requestHandler;
		requestHandler.enable(handler, SIMCONNECT_RECV_ID_SYSTEM_STATE);

		requestHandler.requestSystemState(connection, "AircraftLoaded",
			[](std::string aircraft) {
				std::cout << std::format("Currently loaded aircraft '{}'.\n", aircraft);
			});

		requestHandler.requestSystemState(connection, "DialogMode",
			SimConnect::wrap<bool>([](bool inDialog) {
				std::cout << (inDialog ? "The user is now in a dialog.\n" : "The user is now NOT in a dialog.\n");
			}));

		requestHandler.requestSystemState(connection, "FlightLoaded",
			[](std::string flight) {
				std::cout << std::format("Currently loaded flight '{}'.\n", flight);
			});

		requestHandler.requestSystemState(connection, "FlightPlan",
			[](std::string flightPlan) {
				std::cout << std::format("Currently loaded flightplan '{}'.\n", flightPlan);
			});

		requestHandler.requestSystemState(connection, "Sim",
			SimConnect::wrap<bool>([](bool flying) {
				std::cout << (flying ? "The user is now in control of the aircraft.\n" : "The user is now navigating the UI.\n");
			}));

		requestHandler.requestSystemState(connection, "SimLoaded",
			[](std::string simulator) {
				std::cout << std::format("Currently loaded simulator '{}'.\n", simulator);
			}); // Will cause an exception message

		std::cout << "Handling messages\n";
		handler.handle(30s);
	}
	else {
		std::cerr << "Failed to connect to the simulator.\n";
	}
	return 0;
}