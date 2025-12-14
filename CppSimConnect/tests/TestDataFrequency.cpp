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

#include <simconnect/data_frequency.hpp>

using namespace SimConnect;

// Unit tests for SimConnect::DataFrequency


TEST(TestDataFrequency, DefaultConstructor) {
	const DataFrequency freq;
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_ONCE); // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, 0U);
}

TEST(TestDataFrequency, Once) {
	const DataFrequency freq = DataFrequency::once();
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_ONCE); // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, 0U);
}

TEST(TestDataFrequency, Every) {
	const unsigned long interval = 1000;
	const DataFrequency freq = DataFrequency::every(interval);
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_SECOND); // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, interval);
}

TEST(TestDataFrequency, Seconds) {
	const unsigned long interval = 500;
	const DataFrequency freq = DataFrequency::every(interval).seconds();
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_SECOND); // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, interval);
}

TEST(TestDataFrequency, VisualFrames) {
	const unsigned long interval = 500;
	const DataFrequency freq = DataFrequency::every(interval).visualFrames();
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_VISUAL_FRAME);  // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, interval);
}

TEST(TestDataFrequency, SimFrames) {
	const unsigned long interval = 500;
	const DataFrequency freq = DataFrequency::every(interval).simFrames();
	EXPECT_EQ(freq.period, int(SIMCONNECT_PERIOD_SIM_FRAME));   // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, interval);
}

TEST(TestDataFrequency, Never) {
	const DataFrequency freq = DataFrequency::never();
	EXPECT_EQ(freq.period, SIMCONNECT_PERIOD_NEVER);    // NOLINT(misc-include-cleaner)
	EXPECT_EQ(freq.interval, 0U);
}

