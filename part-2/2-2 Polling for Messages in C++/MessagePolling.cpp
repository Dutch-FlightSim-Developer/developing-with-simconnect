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

#include <simconnect/simple_connection.hpp>
#include <simconnect/polling_handler.hpp>

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


auto main() -> int { // NOLINT(bugprone-exception-escape)
	SimConnect::SimpleConnection connection;
	SimConnect::PollingHandler<SimConnect::SimpleConnection<>> handler(connection);
	handler.autoClosing(true);	// Automatically close the connection if we receive a "Close" message.

	// If we don't know the message, print an error.
	handler.registerDefaultHandler([](const SIMCONNECT_RECV& msg) { // NOLINT(misc-include-cleaner)
			std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
		});

	// Register our handlers for "Open" en "Close"
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen); // NOLINT(misc-include-cleaner)
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose); // NOLINT(misc-include-cleaner)

	std::cout << "Opening connection to the simulator.\n";
	if (connection.open()) {
		std::cout << "Connected to the simulator. Will poll for messages until it quits or you press ^C.\n";

		while (connection.isOpen()) {
			std::cout << "Handling messages for 10 seconds using polling.\n";
            constexpr auto duration = 10s;
			handler.handle(duration);
		}
	}
	else {
		std::cerr << "Failed to open connection to the simulator.\n";
	}
	return 0;
}