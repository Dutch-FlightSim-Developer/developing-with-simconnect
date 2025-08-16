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