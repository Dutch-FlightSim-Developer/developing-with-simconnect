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

#include <simconnect/simconnect_exception.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/requests/system_state_handler.hpp>

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


/** Main entry point. */
auto main () -> int { // NOLINT(bugprone-exception-escape)
	try {
		SimConnect::WindowsEventConnection connection;				// Use a Windows Event.
		SimConnect::WindowsEventHandler handler(connection);		// Use a Windows Event.
		handler.autoClosing(true);

		handler.registerDefaultHandler([](const SIMCONNECT_RECV& msg) { // NOLINT(misc-include-cleaner)
			std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
			});

		handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen); // NOLINT(misc-include-cleaner)
		handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose); // NOLINT(misc-include-cleaner)

		if (connection.open()) {
			SimConnect::SystemStateHandler<SimConnect::WindowsEventHandler<>> requestHandler(handler);

			requestHandler.requestSystemState("AircraftLoaded",
				[](std::string aircraft) {
					std::cout << std::format("Currently loaded aircraft '{}'.\n", aircraft);
				});

			requestHandler.requestSystemState("DialogMode",
				[](bool inDialog) {
					std::cout << (inDialog ? "The simulator is now in dialog mode.\n" : "The simulator is now NOT in dialog mode.\n");
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
					std::cout << (flying ? "The simulator is running.\n" : "The simulator is stopped.\n");
				});

			requestHandler.requestSystemState("SimLoaded",
				[](std::string simulator) {
					std::cout << std::format("Currently loaded simulator '{}'.\n", simulator);
				}); // Will cause an exception message

			std::cout << "Handling messages\n";
            constexpr auto duration = 10s;
			handler.handleFor(duration);
		}
		else {
			std::cerr << "Failed to connect to the simulator.\n";
		}
		if (handler.getHandler(SIMCONNECT_RECV_ID_SYSTEM_STATE).proc()) { // NOLINT(misc-include-cleaner)
			throw SimConnect::SimConnectException("There is still a handler for SystemState messages!");
		}
	}
	catch (const SimConnect::SimConnectException& ex) {
		std::cerr << ex.what() << "\n";
	}
	return 0;
}