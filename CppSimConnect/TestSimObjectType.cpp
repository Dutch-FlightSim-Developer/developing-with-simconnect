/*
 * Copyright (c) 2025. Bert Laverman
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

#include "pch.h"
#include <simconnect/requests/simobject_data_handler.hpp>

using namespace SimConnect;

// Unit tests for SimConnect::SimObjectType

TEST(TestSimObjectType, DefaultConstructor) {
    SimObjectType t;
    EXPECT_EQ(t, SimObjectType::user());
}

TEST(TestSimObjectType, IntConstructor) {
    SimObjectType t(5);
    EXPECT_EQ(t.typeId, 5);
}

TEST(TestSimObjectType, StaticUser) {
    auto t = SimObjectType::user();
    EXPECT_EQ(t.typeId, static_cast<int>(SIMCONNECT_SIMOBJECT_TYPE_USER));
}

TEST(TestSimObjectType, StaticAircraft) {
    auto t = SimObjectType::aircraft();
    EXPECT_EQ(t.typeId, static_cast<int>(SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT));
}

TEST(TestSimObjectType, ConversionOperator) {
    SimObjectType t = SimObjectType::helicopter();
    SIMCONNECT_SIMOBJECT_TYPE raw = t;
    EXPECT_EQ(raw, SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);
}

TEST(TestSimObjectType, OrMethods) {
    constexpr SimObjectTypes t = SimObjectType::boat().orUser().orAircraft();
    unsigned long types = t;
    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::user));
    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::aircraft));
    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::boat));
}
