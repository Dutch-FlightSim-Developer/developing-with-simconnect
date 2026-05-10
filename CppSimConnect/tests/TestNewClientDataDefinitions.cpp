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

#include <cstdint>
#include <numbers>

#include "gtest/gtest.h"

#include <simconnect/data/mapped_client_data_definition.hpp>
#include <simconnect/data/custom_client_data_definition.hpp>
#include <simconnect/data/stateless_client_data_definition.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>

#pragma pack(push, 4)

// NOLINTBEGIN(readability-function-cognitive-complexity)

using namespace SimConnect;
using namespace SimConnect::Data;


// ===========================================================================
// MappedClientDataDefinition
// ===========================================================================

TEST(TestMappedClientDataDefinition, InitialState) {
    struct Data { int32_t a; };
    MappedClientDataDefinition<Data> def;
    EXPECT_FALSE(def.isDefined());
    EXPECT_EQ(def.size(), 0U);
    EXPECT_FALSE(def.useMapping());
}

TEST(TestMappedClientDataDefinition, UseMappingTrueWhenSizeMatchesSizeof) {
    struct Data { int32_t a; float b; };
    MappedClientDataDefinition<Data> def;
    def.addInt32(&Data::a).addFloat32(&Data::b);
    EXPECT_EQ(def.size(), sizeof(Data));
    EXPECT_TRUE(def.useMapping());
}

TEST(TestMappedClientDataDefinition, UseMappingFalseWhenSizeMismatch) {
    struct Data { int32_t a; double b; };
    MappedClientDataDefinition<Data> def;
    def.addInt32(&Data::a);  // only one field — size < sizeof(Data)
    EXPECT_FALSE(def.useMapping());
}

TEST(TestMappedClientDataDefinition, Int32RoundTrip) {
    constexpr int32_t kValue{ 12345678 };
    struct Data { int32_t value; };
    MappedClientDataDefinition<Data> def;
    def.addInt32(&Data::value);

    const Data src{ .value = kValue };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.value, src.value);
}

TEST(TestMappedClientDataDefinition, Int64RoundTrip) {
    constexpr int64_t kValue{ 0x123456789abcdef0LL };
    struct Data { int64_t value; };
    MappedClientDataDefinition<Data> def;
    def.addInt64(&Data::value);

    const Data src{ .value = kValue };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.value, src.value);
}

TEST(TestMappedClientDataDefinition, Float32RoundTrip) {
    constexpr float kValue{ std::numbers::pi_v<float> };
    struct Data { float value; };
    MappedClientDataDefinition<Data> def;
    def.addFloat32(&Data::value);

    const Data src{ .value = kValue };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_FLOAT_EQ(dst.value, src.value);
}

TEST(TestMappedClientDataDefinition, Float64RoundTrip) {
    constexpr double kValue{ std::numbers::e };
    struct Data { double value; };
    MappedClientDataDefinition<Data> def;
    def.addFloat64(&Data::value);

    const Data src{ .value = kValue };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_DOUBLE_EQ(dst.value, src.value);
}

TEST(TestMappedClientDataDefinition, MultipleFieldsRoundTrip) {
    constexpr int32_t kA{ -7 };
    constexpr float   kB{ 1.5F };
    constexpr double  kC{ 9.81 };
    constexpr int8_t  kD{ 99 };
    constexpr int16_t kE{ -500 };
    constexpr int64_t kF{ 0x0EADBEEFCAFELL };

    struct Data { int32_t a; float b; double c; int8_t d; int16_t e; int64_t f; };
    MappedClientDataDefinition<Data> def;
    def.addInt32(&Data::a)
       .addFloat32(&Data::b)
       .addFloat64(&Data::c)
       .addInt8(&Data::d)
       .addInt16(&Data::e)
       .addInt64(&Data::f);

    EXPECT_EQ(def.size(), sizeof(int32_t) + sizeof(float) + sizeof(double) + sizeof(int8_t) + sizeof(int16_t) + sizeof(int64_t));

    const Data src{ .a = kA, .b = kB, .c = kC, .d = kD, .e = kE, .f = kF };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.a, src.a);
    EXPECT_FLOAT_EQ(dst.b, src.b);
    EXPECT_DOUBLE_EQ(dst.c, src.c);
    EXPECT_EQ(dst.d, src.d);
    EXPECT_EQ(dst.e, src.e);
    EXPECT_EQ(dst.f, src.f);
}

TEST(TestMappedClientDataDefinition, AddRawRoundTrip) {
    constexpr int32_t kInnerX{ 42 };
    constexpr float   kInnerY{ 1.5F };
    constexpr int32_t kExtra{ 99 };

    struct Inner { int32_t x; float y; };
    struct Data { Inner inner; int32_t extra; };
    MappedClientDataDefinition<Data> def;
    def.addRaw(&Data::inner).addInt32(&Data::extra);

    EXPECT_EQ(def.size(), sizeof(Inner) + sizeof(int32_t));

    const Data src{ .inner = { .x = kInnerX, .y = kInnerY }, .extra = kExtra };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.inner.x, src.inner.x);
    EXPECT_FLOAT_EQ(dst.inner.y, src.inner.y);
    EXPECT_EQ(dst.extra, src.extra);
}

TEST(TestMappedClientDataDefinition, TaggedMarshalSizeIsDoubleUntagged) {
    constexpr int32_t kA{ 10 };
    constexpr float   kB{ 2.0F };

    struct Data { int32_t a; float b; };
    MappedClientDataDefinition<Data> def;
    def.addInt32(&Data::a).addFloat32(&Data::b);

    const Data src{ .a = kA, .b = kB };
    DataBlockBuilder untagged;
    def.marshal(untagged, src, false);
    DataBlockBuilder tagged;
    def.marshal(tagged, src, true);

    EXPECT_EQ(tagged.size(), untagged.size() + 2 * sizeof(int32_t));
}


// ===========================================================================
// CustomClientDataDefinition
// ===========================================================================

TEST(TestCustomClientDataDefinition, AddFieldRoundTrip) {
    constexpr int     kFactor{ 10 };
    constexpr int32_t kAltitude{ 10000 };
    constexpr double  kSpeed{ 250.0 };

    struct Data { int32_t altitude; double speed; };
    CustomClientDataDefinition<Data> def;
    def.addField(ClientDataType::int32,
        [](Data& d, DataBlockReader& r) { d.altitude = r.readInt32() * kFactor; },
        [](DataBlockBuilder& b, const Data& d) { b.addInt32(d.altitude / kFactor); }
    ).addField(ClientDataType::float64,
        [](Data& d, DataBlockReader& r) { d.speed = r.readFloat64(); },
        [](DataBlockBuilder& b, const Data& d) { b.addFloat64(d.speed); }
    );

    EXPECT_EQ(def.size(), sizeof(int32_t) + sizeof(double));

    const Data src{ .altitude = kAltitude, .speed = kSpeed };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.altitude, src.altitude);
    EXPECT_DOUBLE_EQ(dst.speed, src.speed);
}

TEST(TestCustomClientDataDefinition, AddRawFieldRoundTrip) {
    constexpr int32_t kX{ 7 };
    constexpr double  kY{ 3.14 };

    struct Inner { int32_t x; double y; };
    struct Data { Inner inner; };

    CustomClientDataDefinition<Data> def;
    def.addRawField(sizeof(Inner),
        [](Data& d, DataBlockReader& r) {
            auto bytes = r.readBytes(sizeof(Inner));
            std::memcpy(&d.inner, bytes.data(), sizeof(Inner));
        },
        [](DataBlockBuilder& b, const Data& d) {
            b.addBytes(reinterpret_cast<const uint8_t*>(&d.inner), sizeof(Inner));
        }
    );

    EXPECT_EQ(def.size(), sizeof(Inner));

    const Data src{ .inner = { .x = kX, .y = kY } };
    DataBlockBuilder builder;
    def.marshal(builder, src);

    DataBlockReader reader(builder.dataBlock());
    Data dst{};
    def.unmarshall(reader, dst);
    EXPECT_EQ(dst.inner.x, src.inner.x);
    EXPECT_DOUBLE_EQ(dst.inner.y, src.inner.y);
}

TEST(TestCustomClientDataDefinition, TaggedMarshalSizeIsLargerByOneDatumId) {
    constexpr int32_t kA{ 55 };

    struct Data { int32_t a; };
    CustomClientDataDefinition<Data> def;
    def.addField(ClientDataType::int32,
        [](Data& d, DataBlockReader& r) { d.a = r.readInt32(); },
        [](DataBlockBuilder& b, const Data& d) { b.addInt32(d.a); }
    );

    const Data src{ .a = kA };
    DataBlockBuilder untagged;
    def.marshal(untagged, src, false);
    DataBlockBuilder tagged;
    def.marshal(tagged, src, true);

    EXPECT_EQ(tagged.size(), untagged.size() + sizeof(int32_t));
}


// ===========================================================================
// StatelessClientDataDefinition
// ===========================================================================

TEST(TestStatelessClientDataDefinition, ReceiveOnlyFieldNotMarshalledButUnmarshalled) {
    constexpr int32_t kIncoming{ 42 };

    int32_t received{ 0 };
    StatelessClientDataDefinition def;
    def.addInt32([&received](int32_t v) { received = v; });

    DataBlockBuilder marshalOut;
    def.marshal(marshalOut);
    EXPECT_EQ(marshalOut.size(), 0U);

    DataBlockBuilder src;
    src.addInt32(kIncoming);
    DataBlockReader reader(src.dataBlock());
    def.unmarshall(reader);
    EXPECT_EQ(received, kIncoming);
}

TEST(TestStatelessClientDataDefinition, BidirectionalRoundTrip) {
    constexpr int32_t kInitial{ 99 };

    int32_t state{ kInitial };
    StatelessClientDataDefinition def;
    def.addInt32(
        [&state](int32_t v) { state = v; },
        [&state]() -> int32_t { return state; }
    );

    DataBlockBuilder builder;
    def.marshal(builder);

    state = 0;
    DataBlockReader reader(builder.dataBlock());
    def.unmarshall(reader);
    EXPECT_EQ(state, kInitial);
}

TEST(TestStatelessClientDataDefinition, MultipleFieldsMixedBidirectionalRoundTrip) {
    constexpr int32_t kCount{ 55 };
    constexpr double  kSpeed{ 123.456 };

    int32_t count{ kCount };
    double speed{ kSpeed };

    StatelessClientDataDefinition def;
    def.addInt32(
        [&count](int32_t v) { count = v; },
        [&count]() -> int32_t { return count; }
    ).addFloat64(
        [&speed](double v) { speed = v; },
        [&speed]() -> double { return speed; }
    );

    DataBlockBuilder builder;
    def.marshal(builder);

    count = 0;
    speed = 0.0;
    DataBlockReader reader(builder.dataBlock());
    def.unmarshall(reader);
    EXPECT_EQ(count, kCount);
    EXPECT_DOUBLE_EQ(speed, kSpeed);
}

TEST(TestStatelessClientDataDefinition, MarshalSkipsReceiveOnlyFields) {
    constexpr int32_t kA{ 1 };
    constexpr double  kB{ 2.0 };

    int32_t a{ kA };
    double b{ kB };

    StatelessClientDataDefinition def;
    def.addInt32([&a](int32_t v) { a = v; })
       .addFloat64(
           [&b](double v) { b = v; },
           [&b]() -> double { return b; }
       );

    DataBlockBuilder builder;
    def.marshal(builder);
    EXPECT_EQ(builder.size(), sizeof(double));

    DataBlockReader reader(builder.dataBlock());
    EXPECT_DOUBLE_EQ(reader.readFloat64(), kB);
}

TEST(TestStatelessClientDataDefinition, TaggedMarshalSizeLargerByOneDatumId) {
    constexpr int32_t kState{ 7 };

    int32_t state{ kState };
    StatelessClientDataDefinition def;
    def.addInt32(
        [&state](int32_t v) { state = v; },
        [&state]() -> int32_t { return state; }
    );

    DataBlockBuilder untagged;
    def.marshal(untagged, false);
    DataBlockBuilder tagged;
    def.marshal(tagged, true);

    EXPECT_EQ(tagged.size(), untagged.size() + sizeof(int32_t));
}

// NOLINTEND(readability-function-cognitive-complexity)

#pragma pack(pop)
