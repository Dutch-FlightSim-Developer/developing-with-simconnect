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
#include <simconnect/data/untagged_data_block.hpp>
#include <simconnect/requests/request_handler.hpp>


using namespace std::chrono_literals;

struct AircraftInfo {
    std::string title;
    std::string tailNumber;
    std::string atcId;
    int altitude;
    double latitude;
    double longitude;
};


auto main() -> int {
    SimConnect::WindowsEventConnection connection;
	//SimConnect::WindowsEventHandler handler(connection);
	//handler.autoClosing(true);

 //   if (connection.open()) {
        SimConnect::DataDefinition<AircraftInfo> aircraftDef(connection);
        aircraftDef.add(&AircraftInfo::title, SIMCONNECT_DATATYPE_STRINGV, "title", "string");
        aircraftDef.add(&AircraftInfo::tailNumber, SIMCONNECT_DATATYPE_STRING32, "tailnumber", "string");
        aircraftDef.add(&AircraftInfo::atcId, SIMCONNECT_DATATYPE_STRING64, "atcid", "string");
        aircraftDef.add(&AircraftInfo::latitude, SIMCONNECT_DATATYPE_FLOAT64, "latitude", "degrees");
        aircraftDef.add(&AircraftInfo::longitude, SIMCONNECT_DATATYPE_FLOAT64, "longitude", "degrees");
        aircraftDef.add(&AircraftInfo::altitude, SIMCONNECT_DATATYPE_FLOAT64, "altitude", "feet");

        auto data = SimConnect::Data::UntaggedDataBlockBuilder()
            .addStringV("Cessna 404 Titan")
            .addString32("PH-BLA")
            .addString64("PH-BLA")
            .addLatLonAlt(52.383917, 5.277781, 10000);

        struct AircraftInfo info;
        aircraftDef.extract(data.dataBlock(), info);

        std::cout << std::format("{{ \"title\": \"{}\", \"tailnumber\": \"{}\", \"atcid\": \"{}\", \"altitude\": {}, \"latitude\": {}, \"longitude\": {} }}",
            info.title, info.tailNumber, info.atcId, info.altitude, info.latitude, info.longitude);

 //       SimConnect::RequestHandler requestHandler;

 //       requestHandler.enable(handler, SIMCONNECT_RECV_ID_SIMOBJECT_DATA);

 //       requestHandler.requestData<AircraftInfo>(connection, aircraftDef, [](const AircraftInfo& aircraft) {
 //           std::cout << "Aircraft: " << aircraft.title << "\n"
 //                     << "Tail number: " << aircraft.tailNumber << "\n"
 //                     << "ATC ID: " << aircraft.atcId << "\n";
 //       });
 //       handler.handle(10s);
 //   }
 //   else {
 //       std::cerr << "Failed to open connection to MSFS.\n";
 //   }
    return 0;
}