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

#include <simconnect/data_frequency.hpp>

using namespace SimConnect;

// Unit tests for SimConnect::ClientDataFrequency

TEST(TestClientDataFrequency, DefaultConstructor) {
    const ClientDataFrequency freq;
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_ONCE); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, 0U);
}

TEST(TestClientDataFrequency, Once) {
    const ClientDataFrequency freq = ClientDataFrequency::once();
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_ONCE); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, 0U);
}

TEST(TestClientDataFrequency, Every) {
    const unsigned long interval = 10;
    const ClientDataFrequency freq = ClientDataFrequency::every(interval);
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_SECOND); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, interval);
}

TEST(TestClientDataFrequency, Seconds) {
    const unsigned long interval = 3;
    const ClientDataFrequency freq = ClientDataFrequency::every(interval).seconds();
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_SECOND); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, interval);
}

TEST(TestClientDataFrequency, VisualFrames) {
    const unsigned long interval = 2;
    const ClientDataFrequency freq = ClientDataFrequency::every(interval).visualFrames();
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, interval);
}

TEST(TestClientDataFrequency, OnSet) {
    const ClientDataFrequency freq = ClientDataFrequency::onSet();
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, 0U);
}

TEST(TestClientDataFrequency, Never) {
    const ClientDataFrequency freq = ClientDataFrequency::never();
    EXPECT_EQ(freq.period, SIMCONNECT_CLIENT_DATA_PERIOD_NEVER); // NOLINT(misc-include-cleaner)
    EXPECT_EQ(freq.interval, 0U);
}
