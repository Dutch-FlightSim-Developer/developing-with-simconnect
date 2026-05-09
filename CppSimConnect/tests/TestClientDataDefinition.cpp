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

#include "gtest/gtest.h"

#include <simconnect/simconnect.hpp>

// ---------------------------------------------------------------------------
// sizeOf helper
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, SizeOfInt8)    { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int8),    sizeof(int8_t));  }
TEST(TestClientDataDefinition, SizeOfInt16)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int16),   sizeof(int16_t)); }
TEST(TestClientDataDefinition, SizeOfInt32)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int32),   sizeof(int32_t)); }
TEST(TestClientDataDefinition, SizeOfInt64)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int64),   sizeof(int64_t)); }
TEST(TestClientDataDefinition, SizeOfFloat32) { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::float32), sizeof(float));   }
TEST(TestClientDataDefinition, SizeOfFloat64) { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::float64), sizeof(double));  }
