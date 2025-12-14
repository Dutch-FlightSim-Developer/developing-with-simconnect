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

#include "gtest/gtest.h"


#include <simconnect/simobject_type.hpp>

using namespace SimConnect;

// Unit tests for SimConnect::SimObjectType

TEST(TestSimObjectType, DefaultConstructor) {
    const SimObjectType type;
    EXPECT_EQ(type, SimObjectType::user());
}

TEST(TestSimObjectType, IntConstructor) {
    const SimObjectType type(5);
    EXPECT_EQ(type.typeId, 5);
}

TEST(TestSimObjectType, StaticUser) {
    const SimObjectType type = SimObjectType::user();
    EXPECT_EQ(type.typeId, static_cast<int>(SIMCONNECT_SIMOBJECT_TYPE_USER));   // NOLINT
}

TEST(TestSimObjectType, StaticAircraft) {
    const SimObjectType type = SimObjectType::aircraft();
    EXPECT_EQ(type.typeId, static_cast<int>(SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT));   // NOLINT
}

TEST(TestSimObjectType, ConversionOperator) {
    const SimObjectType type = SimObjectType::helicopter();
    const SIMCONNECT_SIMOBJECT_TYPE raw = type; // NOLINT

    EXPECT_EQ(raw, SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);   // NOLINT
}

TEST(TestSimObjectType, OrMethods) {
    constexpr SimObjectTypes type = SimObjectType::boat().orUser().orAircraft();
    const unsigned long types = type;

    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::user));
    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::aircraft));
    EXPECT_TRUE(static_cast<unsigned long>(types) & static_cast<unsigned long>(SimObjectTypeAsBitField::boat));
}
