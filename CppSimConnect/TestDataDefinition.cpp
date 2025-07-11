/*
 * Copyright (c) 2025. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and limitations under the License.
 */


#include "pch.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/data_definition.hpp>


// We want all our structs to be 4-byte aligned, which is the aliugnment SimConnect expects.
#pragma pack(push, 4)

TEST(TestDataDefinition, TestMarshall) {
    SimConnect::WindowsEventConnection connection;

    constexpr const char* expectedTitle = "Cessna 404 Titan";
    constexpr const char* expectedTailNumber = "PH-BLA";
    constexpr const char* expectedAtcId = "PH-BLA";
    constexpr int expectedAltitude = 10000;
    constexpr double expectedLatitude = 52.383917;
    constexpr double expectedLongitude = 5.277781;
    constexpr double expectedPosLat = 52.37278;
    constexpr double expectedPosLon = 4.89361;
    constexpr double expectedPosAlt = 7.0;

    struct AircraftInfo {
        std::string title;
        std::string tailNumber;
        std::string atcId;
        int altitude;
        double latitude;
        double longitude;
        SIMCONNECT_DATA_LATLONALT pos;
    };

    SimConnect::DataDefinition<AircraftInfo> aircraftDef;
    struct AircraftInfo info {
        expectedTitle, expectedTailNumber, expectedAtcId, expectedAltitude, expectedLatitude, expectedLongitude, { expectedPosLat, expectedPosLon, expectedPosAlt }
    };

    aircraftDef
        .addStringV(&AircraftInfo::title, "title")
        .addString32(&AircraftInfo::tailNumber, "tailnumber")
        .addString64(&AircraftInfo::atcId, "atcid")
        .addFloat64(&AircraftInfo::latitude, "latitude", "degrees")
        .addFloat64("longitude", "degrees",
            [&info](double value) { info.longitude = value; },
            [&info]() -> double { return info.longitude; })
        .addFloat64(&AircraftInfo::altitude, "altitude", "feet")
        .addLatLonAlt("position", "latlonalt",
            [](AircraftInfo& aircraft, const SIMCONNECT_DATA_LATLONALT& pos) { aircraft.pos = pos; },
            [](const AircraftInfo& aircraft) -> SIMCONNECT_DATA_LATLONALT { return aircraft.pos; });

	ASSERT_FALSE(aircraftDef.useMapping()); // Ensure we are not using direct mapping (We have a STRINGV field)

    auto data = SimConnect::Data::DataBlockBuilder();
	aircraftDef.marshall(data, info); // Marshall the data into the data block

    // Now let's see what we got.
    auto reader = SimConnect::Data::DataBlockReader(data.dataBlock());

    ASSERT_EQ(info.title, reader.readStringV());
	ASSERT_EQ(info.tailNumber, reader.readString32());
    ASSERT_EQ(info.atcId, reader.readString64());
    ASSERT_DOUBLE_EQ(info.latitude, reader.readFloat64());
    ASSERT_DOUBLE_EQ(info.longitude, reader.readFloat64());
    ASSERT_EQ(info.altitude, reader.readFloat64());
    
    auto pos = reader.readLatLonAlt();
    ASSERT_DOUBLE_EQ(pos.Latitude, info.pos.Latitude);
    ASSERT_DOUBLE_EQ(pos.Longitude, info.pos.Longitude);
	ASSERT_DOUBLE_EQ(pos.Altitude, info.pos.Altitude);
}


TEST(TestDataDefinition, TestUnMarshall) {
    SimConnect::WindowsEventConnection connection;

    constexpr const char* expectedTitle = "Cessna 404 Titan";
    constexpr const char* expectedTailNumber = "PH-BLA";
    constexpr const char* expectedAtcId = "PH-BLA";
    constexpr int expectedAltitude = 10000;
    constexpr double expectedLatitude = 52.383917;
    constexpr double expectedLongitude = 5.277781;
    constexpr double expectedPosLat = 52.37278;
    constexpr double expectedPosLon = 4.89361;
    constexpr double expectedPosAlt = 7.0;

    struct AircraftInfo {
        std::string title;
        std::string tailNumber;
        std::string atcId;
        int altitude;
        double latitude;
        double longitude;
        SIMCONNECT_DATA_LATLONALT pos;
    };

    SimConnect::DataDefinition<AircraftInfo> aircraftDef;
    struct AircraftInfo info {
        "", "", "", 0, 0.0, 0.0, { 0.0, 0.0, 0.0 }
    };

    aircraftDef
        .addStringV(&AircraftInfo::title, "title")
        .addString32(&AircraftInfo::tailNumber, "tailnumber")
        .addString64(&AircraftInfo::atcId, "atcid")
        .addFloat64(&AircraftInfo::latitude, "latitude", "degrees")
        .addFloat64("longitude", "degrees",
            [&info](double value) { info.longitude = value; },
            [&info]() -> double { return info.longitude; })
        .addFloat64(&AircraftInfo::altitude, "altitude", "feet")
        .addLatLonAlt("position", "latlonalt",
            [](AircraftInfo& aircraft, const SIMCONNECT_DATA_LATLONALT& pos) { aircraft.pos = pos; },
            [](const AircraftInfo& aircraft) -> SIMCONNECT_DATA_LATLONALT { return aircraft.pos; });

    auto data = SimConnect::Data::DataBlockBuilder()
        .addStringV(expectedTitle)
        .addString32(expectedTailNumber)
        .addString64(expectedAtcId)
        .addLatLonAlt(expectedLatitude, expectedLongitude, expectedAltitude)
        .addLatLonAlt(expectedPosLat, expectedPosLon, expectedPosAlt);

    aircraftDef.unmarshall(data.dataBlock(), info);

    ASSERT_EQ(info.title, expectedTitle);
    ASSERT_EQ(info.tailNumber, expectedTailNumber);
    ASSERT_EQ(info.atcId, expectedAtcId);
    ASSERT_EQ(info.altitude, expectedAltitude);
    ASSERT_DOUBLE_EQ(info.latitude, expectedLatitude);
    // longitude is set via lambda
    ASSERT_DOUBLE_EQ(info.longitude, expectedLongitude);
    ASSERT_DOUBLE_EQ(info.pos.Latitude, expectedPosLat);
    ASSERT_DOUBLE_EQ(info.pos.Longitude, expectedPosLon);
    ASSERT_DOUBLE_EQ(info.pos.Altitude, expectedPosAlt);
}


// Test marshalling of a mappable struct with int32, int64, float32, float64, and string8 fields
TEST(TestDataDefinition, Marshall_MappableStruct_AllTypes) {
    struct Mappable {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        char str8[8];
    };
    constexpr int32_t expectedI32 = 1234;
    constexpr int64_t expectedI64 = 0x123456789abcdef0;
    constexpr float expectedF32 = 3.14f;
    constexpr double expectedF64 = 2.718281828459;
    constexpr const char* expectedStr8 = "ABC";

    SimConnect::DataDefinition<Mappable> def;
    def.addInt32(&Mappable::i32, "i32", "")
       .addInt64(&Mappable::i64, "i64", "")
       .addFloat32(&Mappable::f32, "f32", "")
       .addFloat64(&Mappable::f64, "f64", "")
       .add(&Mappable::str8, SIMCONNECT_DATATYPE_STRING8, "str8");

    ASSERT_TRUE(def.useMapping());
    ASSERT_EQ(def.size(), sizeof(Mappable));

    constexpr Mappable src{expectedI32, expectedI64, expectedF32, expectedF64, {'A','B','C','\0'}};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();

    // Check the raw data for each field
    int32_t i32 = 0;
    int64_t i64 = 0;
    float f32 = 0.0f;
    double f64 = 0.0;
    char str8[8] = {0};
    size_t offset = 0;
    std::memcpy(&i32, data.data() + offset, sizeof(i32)); offset += sizeof(i32);
    std::memcpy(&i64, data.data() + offset, sizeof(i64)); offset += sizeof(i64);
    std::memcpy(&f32, data.data() + offset, sizeof(f32)); offset += sizeof(f32);
    std::memcpy(&f64, data.data() + offset, sizeof(f64)); offset += sizeof(f64);
    std::memcpy(&str8, data.data() + offset, sizeof(str8));

    ASSERT_EQ(i32, expectedI32);
    ASSERT_EQ(i64, expectedI64);
    ASSERT_FLOAT_EQ(f32, expectedF32);
    ASSERT_DOUBLE_EQ(f64, expectedF64);
    ASSERT_EQ(std::string(str8), std::string(expectedStr8));
}

// Test unmarshalling of a mappable struct with int32, int64, float32, float64, and string8 fields
TEST(TestDataDefinition, Unmarshall_MappableStruct_AllTypes) {
    struct Mappable {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        char str8[8];
    };
    constexpr int32_t expectedI32 = 5678;
    constexpr int64_t expectedI64 = 0x0fedcba987654321;
    constexpr float expectedF32 = 1.23f;
    constexpr double expectedF64 = 9.87654321;
    constexpr const char* expectedStr8 = "XYZ";

    SimConnect::DataDefinition<Mappable> def;
    def.addInt32(&Mappable::i32, "i32", "")
       .addInt64(&Mappable::i64, "i64", "")
       .addFloat32(&Mappable::f32, "f32", "")
       .addFloat64(&Mappable::f64, "f64", "")
       .add(&Mappable::str8, SIMCONNECT_DATATYPE_STRING8, "str8");

    ASSERT_TRUE(def.useMapping());
    ASSERT_EQ(def.size(), sizeof(Mappable));

    // Prepare a data block with the expected values
    std::vector<uint8_t> data(sizeof(Mappable), 0);
    size_t offset = 0;
    std::memcpy(data.data() + offset, &expectedI32, sizeof(expectedI32)); offset += sizeof(expectedI32);
    std::memcpy(data.data() + offset, &expectedI64, sizeof(expectedI64)); offset += sizeof(expectedI64);
    std::memcpy(data.data() + offset, &expectedF32, sizeof(expectedF32)); offset += sizeof(expectedF32);
    std::memcpy(data.data() + offset, &expectedF64, sizeof(expectedF64)); offset += sizeof(expectedF64);
    char str8[8] = {0};
    strncpy_s(str8, sizeof(str8), expectedStr8, 8);
    std::memcpy(data.data() + offset, str8, sizeof(str8));

    Mappable dst{0, 0, 0.0f, 0.0, {0}};
    SimConnect::Data::DataBlockReader reader(std::span<const uint8_t>(data.data(), data.size()));
    def.unmarshall(reader, dst);

    ASSERT_EQ(dst.i32, expectedI32);
    ASSERT_EQ(dst.i64, expectedI64);
    ASSERT_FLOAT_EQ(dst.f32, expectedF32);
    ASSERT_DOUBLE_EQ(dst.f64, expectedF64);
    ASSERT_EQ(std::string(dst.str8), std::string(expectedStr8));
}
