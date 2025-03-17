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
#include <windows.h>
#include <SimConnect.h>
#pragma warning(pop)

#include <stdio.h>
#include <time.h>


enum REQUEST_IDS : SIMCONNECT_DATA_REQUEST_ID{
	REQ_AIRCRAFT_LOADED,
	REQ_FLIGHT_LOADED,
	REQ_FLIGHTPLAN_LOADED,
	REQ_DIALOG_MODE,
	REQ_SIM_STATE,
	REQ_SIM_LOADED
};


static HANDLE hSimConnect{ nullptr };		// SimConnect handle


/**
 * Request the system state.
 * @param name The name of the system state
 */
static void requestSystemState(DWORD reqId, const char* name) {
	if (SUCCEEDED(SimConnect_RequestSystemState(hSimConnect, reqId, name))) {
		printf("SystemState '%s' requested with RequestID %d.\n", name, reqId);
	}
	else {
		printf("Request for '%s' AircraftLoaded failed.\n", name);
	}
}


static bool connected{ true };		// If `true`, we are currently connected to the simulator.


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

            switch (msg->dwRequestID) {

            case REQ_AIRCRAFT_LOADED:
                printf("AircraftLoaded: '%s'\n", msg->szString);
                break;

            case REQ_FLIGHT_LOADED:
                printf("FlightLoaded: '%s'\n", msg->szString);
                break;

            case REQ_FLIGHTPLAN_LOADED:
                printf("FlightPlan: '%s'\n", msg->szString);
                break;

            case REQ_DIALOG_MODE:
                printf("DialogMode: %d\n", msg->dwInteger);
                break;

            case REQ_SIM_STATE:
                printf("Sim State: %d\n", msg->dwInteger);
                break;

            case REQ_SIM_LOADED:
                printf("Sim Loaded: '%s'\n", msg->szString);
                break;

            default:
                printf("SystemState for request %d received. (%d, %f, '%s')\n",
                    msg->dwRequestID, (int)msg->dwInteger, msg->fFloat, msg->szString);
                break;
            }

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
int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "RequestSystemState", nullptr, 0, nullptr, 0))) {
		printf("Connected to the Simulator.\n");

		requestSystemState(REQ_AIRCRAFT_LOADED, "AircraftLoaded");
		requestSystemState(REQ_DIALOG_MODE, "DialogMode");
		requestSystemState(REQ_FLIGHT_LOADED, "FlightLoaded");
		requestSystemState(REQ_FLIGHTPLAN_LOADED, "FlightPlan");
		requestSystemState(REQ_SIM_STATE, "Sim");
		requestSystemState(REQ_SIM_LOADED, "SimLoaded"); // Will cause an exception

		printf("Handling messages for 10 seconds.\n");
		const time_t start{ time(nullptr) };

		while (connected && (difftime(time(nullptr), start) < 10)) {
			processMessages();

			if (connected) {
				Sleep(100);		// Try to convince our protection we're not malware
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