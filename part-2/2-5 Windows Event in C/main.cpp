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

#pragma warning(push, 3)

// The other headers need this one, but don't include it themselves...
#include <windows.h> // NOLINT(misc-include-cleaner)

#include <minwindef.h>
#include <winnt.h>
#include <synchapi.h>
#include <winerror.h>

#include "SimConnect.h"

#pragma warning(pop)

#include <ctime>
#include <format>
#include <iostream>


/**
 * Process all currently available messages.
 */
static bool processMessages(HANDLE hSimConnect) {
    bool connected{ true };
	while(connected) {
		SIMCONNECT_RECV* data{ nullptr };
		DWORD len{ 0 };
		const HRESULT result = SimConnect_GetNextDispatch(hSimConnect, &data, &len);

		if (FAILED(result)) {
			break;
		}

		switch (data->dwID) {

		case SIMCONNECT_RECV_ID_OPEN:				// We have an active connection to the simulator.
		{
			const auto* msg = static_cast<const SIMCONNECT_RECV_OPEN*>(data); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
			std::cout << std::format("Connected to simulator {} version {}.{}. (build {}.{})\n",
				msg->szApplicationName,
				msg->dwApplicationVersionMajor, msg->dwApplicationVersionMinor,
				msg->dwApplicationBuildMajor, msg->dwApplicationBuildMinor);
			std::cout << std::format("  using SimConnect version {}.{}. (build {}.{})\n",
				msg->dwSimConnectVersionMajor, msg->dwSimConnectVersionMinor,
				msg->dwSimConnectBuildMajor, msg->dwSimConnectBuildMinor);
		}
			break;

		case SIMCONNECT_RECV_ID_QUIT:				// The simulator is shutting down.
			std::cout << "Received quit message from simulator.\n";
			connected = false;
			break;

		default:
			std::cout << std::format("Received an unknown message with type {}. (size {} bytes)\n", data->dwID, len);
			break;
		}
	}
    return connected;
}


/**
 * The main entry point.
 * @param argc The number of command line arguments
 * @param argv The command line arguments
 * @return The exit code
 */
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) // NOLINT(bugprone-exception-escape)
{
    HANDLE hSimConnect{ nullptr };

	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "RequestSystemState", nullptr, 0, nullptr, 0))) {
		std::cout << "Connected to the Simulator.\n";

		std::cout << "Handling messages for 10 seconds.\n";
		const time_t start{ time(nullptr) };

        const double waitSeconds{ 10.0 };
		while ((difftime(time(nullptr), start) < waitSeconds) && processMessages(hSimConnect)) {
            const DWORD waitMilliseconds{ 100 };
            Sleep(waitMilliseconds);		// Try to convince our protection we're not malware
		}

		SimConnect_Close(hSimConnect);
		std::cout << "Disconnected from the simulator.\n";
	}
	else {
		std::cerr << "Failed to connect to the simulator.\n";
	}
	return 0;
}