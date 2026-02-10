/*
 * Copyright (c) 2026. Bert Laverman
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

// An MSFS 2020 Console Application, linked with the static SimConnect library.



#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#include <iostream>
#include <format>
#include <string>


constexpr static const char* appName = "SimConnect Console Application";
static HANDLE hSimConnect{ nullptr };
static HANDLE hEvent{ nullptr };


static bool connect()
{
	if (hEvent == nullptr)
	{
		hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hEvent == nullptr) {
			std::cerr << std::format("Failed to create event: 0x{:08X}.\n", GetLastError());
			return false;
		}
	}

	HRESULT hr = SimConnect_Open(&hSimConnect, appName, nullptr, 0, hEvent, 0);
	if (FAILED(hr))
	{
		std::cerr << std::format("Failed to connect to SimConnect: 0x{:08X}\n", hr);
		return false;
	}

	return true;
}


static void disconnect()
{
	if (hSimConnect != nullptr) {
		SimConnect_Close(hSimConnect);
		hSimConnect = nullptr;
	}
	if (hEvent != nullptr) {
		CloseHandle(hEvent);
		hEvent = nullptr;
	}
}


auto main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) -> int
{
	if (!connect()) {
		return -1;
	}
	std::cout << "Connected to MSFS 2020!\n";

	// Your SimConnect code goes here...

	disconnect();

	return 0;
}