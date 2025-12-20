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

#include <winnt.h>
#include <winerror.h>

#include "SimConnect.h"

#pragma warning(pop)


#include <iostream>


/**
 * Run our test.
 */
auto main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    HANDLE hSimConnect{ nullptr };
    HRESULT result = SimConnect_Open(&hSimConnect, "MessagePolling", nullptr, 0, nullptr, 0);

    if (SUCCEEDED(result)) {
		std::cout << "Connected to Flight Simulator!\n";

        result = SimConnect_Close(hSimConnect);
        if (SUCCEEDED(result)) {
            std::cout << "Disconnected from Flight Simulator.\n";
        }
        else {
            std::cerr << "SimConnect_Close failed. (result = 0x" << std::hex << result << ")\n";
        }
	}
    else {
		std::cerr << "Failed to connect to Flight Simulator! (result = 0x" << std::hex << result << ")\n";
	}

	return 0;
}
