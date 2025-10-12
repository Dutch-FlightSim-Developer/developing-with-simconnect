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
#include <simconnect/events/system_event_handler.hpp>

#include <iostream>


using namespace std::chrono_literals;



/**
 * Return a pretty formatted version string.
 * @param major major version number. If 0, return "Unknown".
 * @param minor minor version number. If 0, return just the major version number.
 * @return version string.
 */
static std::string version(int major, int minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


/**
 * Handle the SIMCONNECT_RECV_OPEN message.
 */
static void handleOpen(const SIMCONNECT_RECV_OPEN& msg) {
	std::cout << "Connected to " << msg.szApplicationName
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << std::endl
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << std::endl
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << std::endl
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << std::endl;
}


/**
 * Handle the SIMCONNECT_RECV_QUIT message.
 */
static void handleClose([[maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
}


/**
 * Handle the SIMCONNECT_RECV_EVENT message.
 */
static void handleEvent(const SIMCONNECT_RECV_EVENT& msg) {
	if (msg.uGroupID == SIMCONNECT_RECV_EVENT::UNKNOWN_GROUP) {
		std::cout << std::format("Received event '{}' ({}): dwData = {}\n",
			SimConnect::event::get(msg.uEventID).name(), msg.uEventID, msg.dwData);
	}
	else {
		std::cout << std::format("Received event '{}' ({}) in group {}: dwData = {}\n",
			SimConnect::event::get(msg.uEventID).name(), msg.uEventID, msg.uGroupID, msg.dwData);
	}
}


/**
 * Demonstrate how to subscribe to system events.
 */
auto main() -> int {
	SimConnect::WindowsEventConnection connection;
	SimConnect::WindowsEventHandler handler(connection);
	handler.autoClosing(true);

	handler.setDefaultHandler([](const SIMCONNECT_RECV& msg) {
		std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
	});
	handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, handleOpen);
	handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, handleClose);
	handler.registerHandler<SIMCONNECT_RECV_EVENT>(SIMCONNECT_RECV_ID_EVENT, handleEvent);

	if (connection.open()) {
		SimConnect::SystemEventHandler<SimConnect::WindowsEventHandler<>> eventHandler;
		eventHandler.enable(handler);

		eventHandler.subscribeToSystemEvent(connection, SimConnect::Events::sim(), [](const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'Sim' event with value {}.\n", msg.dwData);
		});
		eventHandler.subscribeToSystemEvent(connection, SimConnect::Events::simStart(), []([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'SimStart' event.\n");
			});
		eventHandler.subscribeToSystemEvent(connection, SimConnect::Events::simStop(), []([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'SimStop' event.\n");
			});
		eventHandler.subscribeToSystemEvent(connection, SimConnect::Events::pause(), [](const SIMCONNECT_RECV_EVENT& msg) {
			std::cout << std::format("Received a 'Pause' event with value {}.\n", msg.dwData);
			});

		std::cout << "\n\nHandling messages for 30 seconds.\n";
		handler.handle(30s);
	}
	else {
		std::cerr << "Failed to connect to simulator.\n";
	}

	return 0;
}