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
#include <algorithm>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/data_definition.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/requests/request_handler.hpp>


using namespace std::chrono_literals;

struct AircraftInfo {
    std::string title;
    std::string tailNumber;
    std::string atcId;
    int altitude;
    double latitude;
    double longitude;
    SIMCONNECT_DATA_LATLONALT pos;
};


auto main() -> int {
    SimConnect::WindowsEventConnection connection;

    SimConnect::DataDefinition<AircraftInfo> aircraftDef(connection);
    struct AircraftInfo info {
        "", "", "", 0, 0.0, 0.0, { 0.0, 0.0, 0.0 }
    };

    aircraftDef
        .addStringV(&AircraftInfo::title, "title", "string")
        .addString32(&AircraftInfo::tailNumber, "tailnumber", "string")
        .addString64(&AircraftInfo::atcId, "atcid", "string")
        .addFloat64(&AircraftInfo::latitude, "latitude", "degrees")
        .addFloat64("longitude", "degrees",
            [&info](double value) { info.longitude = value; },
            [&info]() -> double { return info.longitude; })
        .addFloat64(&AircraftInfo::altitude, "altitude", "feet")
        .addLatLonAlt("position", "latlonalt",
            [](AircraftInfo& aircraft, const SIMCONNECT_DATA_LATLONALT& pos) { aircraft.pos = pos; },
            [](const AircraftInfo& aircraft) -> SIMCONNECT_DATA_LATLONALT { return aircraft.pos; });

    auto data = SimConnect::Data::DataBlockBuilder()
        .addStringV("Cessna 404 Titan")
        .addString32("PH-BLA")
        .addString64("PH-BLA")
        .addLatLonAlt(52.383917, 5.277781, 10000)
        .addLatLonAlt(52.37278, 4.89361, 7);

    aircraftDef.unmarshall(data.dataBlock(), info);

    std::cout << std::format("{{ \"title\": \"{}\", \"tailnumber\": \"{}\", \"atcid\": \"{}\", \"altitude\": {}, \"latitude\": {}, \"longitude\": {}, \"pos\": {{ \"latitude\": {}, \"longitude\": {}, \"altitude\": {} }} }}\n\n",
        info.title, info.tailNumber, info.atcId, info.altitude, info.latitude, info.longitude, info.pos.Latitude, info.pos.Longitude, info.pos.Altitude);

    auto data2 = SimConnect::Data::DataBlockBuilder();
    aircraftDef.marshall(data2, info);

    std::cout
        << std::format("{} bytes in, {} bytes out.\n", data.dataBlock().size(), data2.dataBlock().size())
        << ((std::equal(data.dataBlock().begin(), data.dataBlock().end(), data2.dataBlock().begin())) ? "They are EQUAL!\n" : "They are NOT EQUAL!\n");


    return 0;
}