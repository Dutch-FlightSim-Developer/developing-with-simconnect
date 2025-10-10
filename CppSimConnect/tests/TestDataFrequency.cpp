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
#include <simconnect/data_frequency.hpp>

using namespace SimConnect;

// Unit tests for SimConnect::DataFrequency


TEST(TestDataFrequency, DefaultConstructor) {
	DataFrequency df;
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_ONCE);
	EXPECT_EQ(df.interval, 0u);
}

TEST(TestDataFrequency, Once) {
	DataFrequency df = DataFrequency::once();
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_ONCE);
	EXPECT_EQ(df.interval, 0u);
}

TEST(TestDataFrequency, Every) {
	unsigned long interval = 1000;
	DataFrequency df = DataFrequency::every(interval);
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_SECOND);
	EXPECT_EQ(df.interval, interval);
}

TEST(TestDataFrequency, Seconds) {
	unsigned long interval = 500;
	DataFrequency df = DataFrequency::every(interval).seconds();
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_SECOND);
	EXPECT_EQ(df.interval, interval);
}

TEST(TestDataFrequency, VisualFrames) {
	unsigned long interval = 500;
	DataFrequency df = DataFrequency::every(interval).visualFrames();
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_VISUAL_FRAME);
	EXPECT_EQ(df.interval, interval);
}

TEST(TestDataFrequency, SimFrames) {
	unsigned long interval = 500;
	DataFrequency df = DataFrequency::every(interval).simFrames();
	EXPECT_EQ(df.period, int(SIMCONNECT_PERIOD_SIM_FRAME));
	EXPECT_EQ(df.interval, interval);
}

TEST(TestDataFrequency, Never) {
	DataFrequency df = DataFrequency::never();
	EXPECT_EQ(df.period, SIMCONNECT_PERIOD_NEVER);
	EXPECT_EQ(df.interval, 0u);
}

