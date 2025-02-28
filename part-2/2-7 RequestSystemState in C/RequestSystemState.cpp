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
#include <SimConnect.h>

#include <stdio.h>
#include <time.h>


static HANDLE hSimConnect;		// SimConnect handle


/**
 * Get the next request ID.
 * @return The next request ID
 */
static int nextReqId() {
	static int reqId{ 0 };			// The last used request ID

	return ++reqId;
}


/**
 * Request the system state.
 * @param name The name of the system state
 */
static void requestSystemState(const char* name) {
	int reqId{ nextReqId() };

	if (SUCCEEDED(SimConnect_RequestSystemState(hSimConnect, reqId, name))) {
		printf("SystemState '%s' requested with RequestID %d.\n", name, reqId);
	}
	else {
		printf("Request for '%s' AircraftLoaded failed.\n", name);
	}
}


static bool connected = true;		// If `true`, we are currently connected to the simulator.


/**
 * Process all currently available messages.
 */
static void processMessages() {
	while(connected) {
		SIMCONNECT_RECV* data{ nullptr };
		DWORD len{ 0 };
		HRESULT hr = SimConnect_GetNextDispatch(hSimConnect, &data, &len);
		if (FAILED(hr)) {
			break;
		}
		switch (data->dwID) {

		case SIMCONNECT_RECV_ID_OPEN:				// We have an active connection to the simulator.
		{
			SIMCONNECT_RECV_OPEN* msg = (SIMCONNECT_RECV_OPEN*)data;
			printf("Connected to simulator %s version %d.%d. (build %d.%d)\n",
				msg->szApplicationName,
				msg->dwApplicationVersionMajor, msg->dwApplicationVersionMinor,
				msg->dwApplicationBuildMajor, msg->dwApplicationBuildMinor);
			printf("  using SimConnect version %d.%d. (build %d.%d)\n",
				msg->dwSimConnectVersionMajor, msg->dwSimConnectVersionMinor,
				msg->dwSimConnectBuildMajor, msg->dwSimConnectBuildMinor);
		}
			break;

		case SIMCONNECT_RECV_ID_QUIT:				// The simulator is shutting down.
			printf("Received quit message from simulator.\n");
			connected = false;
			break;

		case SIMCONNECT_RECV_ID_SYSTEM_STATE:		// A system state has been received
		{
			SIMCONNECT_RECV_SYSTEM_STATE* msg = (SIMCONNECT_RECV_SYSTEM_STATE*)data;
			printf("SystemState for request %d received. (%d, %f, '%s')\n",
				msg->dwRequestID, (int)msg->dwInteger, msg->fFloat, msg->szString);
		}
			break;

		default:
			printf("Received an unknown message with type %d. (size %d bytes)\n", data->dwID, len);
			break;
		}
	}
}


/**
 * The main entry point.
 * @param argc The number of command line arguments
 * @param argv The command line arguments
 * @return The exit code
 */
int main(int argc, const char* argv[])
{
	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "RequestSystemState", nullptr, 0, nullptr, 0))) {
		printf("Connected to the Simulator.\n");

		requestSystemState("AircraftLoaded");
		requestSystemState("DialogMode");
		requestSystemState("FlightLoaded");
		requestSystemState("FlightPlan");
		requestSystemState("Sim");
		requestSystemState("SimLoaded"); // Will cause an exception

		printf("Handling messages for 10 seconds.\n");
		const time_t start = time(nullptr);
		while (connected && (difftime(time(nullptr), start) < 10)) {
			processMessages();
			if (time(nullptr) - start > 5) {
				connected = false;
			}
		}

		SimConnect_Close(hSimConnect);
		printf("Disconnected from the simulator.\n");
	}
	else {
		printf("Failed to connect to the simulator.\n");
	}
	return 0;
}