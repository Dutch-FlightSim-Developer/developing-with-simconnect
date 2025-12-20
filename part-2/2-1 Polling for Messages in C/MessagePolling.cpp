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

#include <format>
#include <iostream>


/**
 * Connect to Microsoft Flight Simulator.
 *
 * @return true on success.
 */
bool connect(HANDLE& hSimConnect)
{
	HRESULT result = SimConnect_Open(&hSimConnect, "MessagePolling", nullptr, 0, nullptr, 0);
	if (SUCCEEDED(result)) {
		std::cout << "Connected to Flight Simulator!\n";
	}
	else {
		std::cerr << std::format("Failed to connect to Flight Simulator! (result = 0x{:08x})\n", result);
	}
	return SUCCEEDED(result);
}


/**
 * Handle messages from the simulator, simply "polling". We first drain the queue of messages, then wait
 * 100ms before we try again.
 *
 * When a message is received, we only handle the "Open" and "Quit" ones.
 */
void handle_messages(HANDLE hSimConnect)
{
    bool connected{ true };
	while (connected) {
		SIMCONNECT_RECV* pData{ nullptr };
		DWORD cbData{ 0 };
		HRESULT result{ S_OK };

		while (SUCCEEDED(result = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
			switch (pData->dwID) {
			case SIMCONNECT_RECV_ID_OPEN:
				{
					const auto* pOpen = static_cast<const SIMCONNECT_RECV_OPEN*>(pData); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
					std::cout << std::format("Connected to '{}' version {}.{} (build {}.{})\n",
                        pOpen->szApplicationName,
                        pOpen->dwApplicationVersionMajor, pOpen->dwApplicationVersionMinor,
                        pOpen->dwApplicationBuildMajor, pOpen->dwApplicationBuildMinor);
					std::cout << std::format("  using SimConnect version {}.{} (build {}.{})\n", pOpen->dwSimConnectVersionMajor, pOpen->dwSimConnectVersionMinor, pOpen->dwSimConnectBuildMajor, pOpen->dwSimConnectBuildMinor);
				}
				break;

			case SIMCONNECT_RECV_ID_QUIT:
				std::cout << "Simulator shutting down.\n";
				connected = false;
				break;

			default:
				std::cout << std::format("Ignoring message of type {} (length {} bytes)\n", pData->dwID, cbData);
				break;
			}
		}
		if (connected) {
            const DWORD waitMilliseconds{ 100 };
			Sleep(waitMilliseconds);		// Try to convince our protection we're not malware
		}
	}
}


/**
 * Close the connection.
 */
void close(HANDLE hSimConnect)
{
	if (FAILED(SimConnect_Close(hSimConnect))) {
		std::cerr << "SimConnect_Close failed.\n";
	}
}


/**
 * Run our test.
 */
auto main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    HANDLE hSimConnect{ nullptr };
	if (connect(hSimConnect)) {
		handle_messages(hSimConnect);
		close(hSimConnect);
	}

	return 0;
}
