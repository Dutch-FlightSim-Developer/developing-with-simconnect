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

inline static std::string version(int major, int minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}

static void handleOpen(SIMCONNECT_RECV_OPEN& msg) {
	std::cout << "Connected to " << msg.szApplicationName
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << std::endl
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << std::endl
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << std::endl
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << std::endl;
}

static void handleClose(SIMCONNECT_RECV_QUIT& msg) {
	std::cout << "Simulator shutting down.\n";
}

static void requestSystemState(SimConnect::Connection& connection, const char* stateName) {
	static DWORD requestID{ 0 };

	auto hr = SimConnect_RequestSystemState(connection, ++requestID, stateName);
	if (SUCCEEDED(hr)) {
		std::cout << std::format("Requested system state '{}' using request ID {}\n", stateName, requestID);
	}
	else {
		std::cerr << std::format("Failed to request system state '{}': {}\n", stateName, hr);
	}
}

static void handleSystemState(SIMCONNECT_RECV_SYSTEM_STATE& msg) {
	std::cout << std::format("Received system state for request {}: {}, {}, '{}'\n", (int)msg.dwRequestID, msg.dwInteger, msg.fFloat, msg.szString);
}

auto main () -> int {
	SimConnect::WindowsEventConnection connection;
	SimConnect::WindowsEventHandler handler(connection);
	bool connected{ false };

	handler.setDefaultHandler([](SIMCONNECT_RECV* msg, DWORD len) {
			std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg->dwID, len);
		});
	handler.registerHandler(SIMCONNECT_RECV_ID_OPEN, [](SIMCONNECT_RECV* msg, DWORD len) {
			handleOpen(*reinterpret_cast<SIMCONNECT_RECV_OPEN*>(msg));
		});
	handler.registerHandler(SIMCONNECT_RECV_ID_QUIT, [&connected](SIMCONNECT_RECV* msg, DWORD len) {
			handleClose(*reinterpret_cast<SIMCONNECT_RECV_QUIT*>(msg));
			connected = false;
		});
	handler.registerHandler(SIMCONNECT_RECV_ID_SYSTEM_STATE, [](SIMCONNECT_RECV* msg, DWORD len) {
			handleSystemState(*reinterpret_cast<SIMCONNECT_RECV_SYSTEM_STATE*>(msg));
		});

	connected = connection.open();
	if (connected) {
		requestSystemState(connection, "AircraftLoaded");
		requestSystemState(connection, "DialogMode");
		requestSystemState(connection, "FlightLoaded");
		requestSystemState(connection, "FlightPlan");
		requestSystemState(connection, "Sim");
		requestSystemState(connection, "SimLoaded");

		std::cout << "Handling messages\n";
		handler.handle(30s);
	}
	return 0;
}