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


 #include <iostream>
 #include <string>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/data_definition.hpp>
#include <simconnect/requests/request_handler.hpp>


using namespace std::chrono_literals;

struct AircraftInfo {
    std::string title;
    std::string tailNumber;
    std::string atcId;
};


auto main() -> int {
	SimConnect::WindowsEventConnection connection;
	SimConnect::WindowsEventHandler handler(connection);
	handler.autoClosing(true);

    if (connection.open()) {
        SimConnect::DataDefinition<AircraftInfo> aircraftDef(connection);
        aircraftDef.add(&AircraftInfo::title, "Title", "string");
        aircraftDef.add(&AircraftInfo::tailNumber, "TailNumber", "string");
        aircraftDef.add(&AircraftInfo::atcId, "ATCId", "string");
    
        SimConnect::RequestHandler requestHandler;

        requestHandler.enable(handler, SIMCONNECT_RECV_ID_SIMOBJECT_DATA);

        requestHandler.requestData<AircraftInfo>(connection, aircraftDef, [](const AircraftInfo& aircraft) {
            std::cout << "Aircraft: " << aircraft.title << "\n"
                      << "Tail number: " << aircraft.tailNumber << "\n"
                      << "ATC ID: " << aircraft.atcId << "\n";
        });
        handler.handle(10s);
    }
    else {
        std::cerr << "Failed to open connection to MSFS.\n";
    }
    return 0;
}