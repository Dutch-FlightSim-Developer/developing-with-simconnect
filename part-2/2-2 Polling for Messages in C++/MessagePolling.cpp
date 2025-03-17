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
inline static std::string version(int major, int minor) {
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
static void handleOpen(const SIMCONNECT_RECV_OPEN& msg) {
	std::cout << "Connected to " << msg.szApplicationName
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << std::endl
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << std::endl
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << std::endl
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << std::endl;
}


/**
 * Tell the use the simulator is shutting down.
 * 
 * @param msg The message received.
 */
static void handleClose([[maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
}


auto main() -> int {
	SimConnect::SimpleConnection connection;
	SimConnect::PollingHandler handler(connection);
	handler.autoClosing(true);	// Automatically close the connection if we receive a "Close" message.

	// If we don't know the message, print an error.
	handler.setDefaultHandler([](const SIMCONNECT_RECV* msg, DWORD len) {
			std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg->dwID, len);
		});

	// Register our handlers for "Open" en "Close"
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen);
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose);

	std::cout << "Opening connection to the simulator.\n";
	if (connection.open()) {
		std::cout << "Connected to the simulator. Will poll for messages until it quits or you press ^C.\n";

		while (connection.isOpen()) {
			std::cout << "Handling messages for 10 seconds using polling.\n";
			handler.handle(10s);
		}
	}
	else {
		std::cerr << "Failed to open connection to the simulator.\n";
	}
	return 0;
}