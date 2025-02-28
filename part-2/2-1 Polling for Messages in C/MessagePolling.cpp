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


#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "SimConnect.h"


static HANDLE hSimConnect;
static bool connected = false;


/**
 * Connect to Microsoft Flight Simulator.
 *
 * @return true on success.
 */
bool connect()
{
	HRESULT hr;
	connected = SUCCEEDED(hr = SimConnect_Open(&hSimConnect, "MessagePolling", nullptr, 0, nullptr, 0));
	if (connected) {
		printf("Connected to Flight Simulator!\n");
	}
	else {
		printf("Failed to connect to Flight Simulator! (hr = 0x%08x)\n", hr);
	}
	return connected;
}


/**
 * Handle messages from the simulator, simply "polling". We first drain the queue of messages, then wait
 * 100ms before we try again.
 *
 * When a message is received, we only handle the "Open" and "Quit" ones.
 */
void handle_messages()
{
	while (connected) {
		SIMCONNECT_RECV* pData;
		DWORD cbData;
		HRESULT hr;
		while (SUCCEEDED(hr = SimConnect_GetNextDispatch(hSimConnect, &pData, &cbData))) {
			switch (pData->dwID) {
			case SIMCONNECT_RECV_ID_OPEN:
			{
				SIMCONNECT_RECV_OPEN* pOpen = (SIMCONNECT_RECV_OPEN*)pData;
				printf("Connected to '%s' version %d.%d (build %d.%d)\n", pOpen->szApplicationName, pOpen->dwApplicationVersionMajor, pOpen->dwApplicationVersionMinor, pOpen->dwApplicationBuildMajor, pOpen->dwApplicationBuildMinor);
				printf("  using SimConnect version %d.%d (build %d.%d)\n", pOpen->dwSimConnectVersionMajor, pOpen->dwSimConnectVersionMinor, pOpen->dwSimConnectBuildMajor, pOpen->dwSimConnectBuildMinor);
				break;
			}

			case SIMCONNECT_RECV_ID_QUIT:
				printf("Simulator stopped stopped.\n");
				connected = false;
				break;

			default:
				printf("Ignoring message of type %d (length %d bytes)\n", pData->dwID, cbData);
				break;
			}
		}
		if (connected) {
			Sleep(100);
		}
	}
}


/**
 * Close the connection.
 */
void close()
{
	if (FAILED(SimConnect_Close(hSimConnect))) {
		printf("SimConnect_Close failed.\n");
	}
}

int main()
{
	if (connect()) {
		handle_messages();
		close();
	}

	return 0;
}