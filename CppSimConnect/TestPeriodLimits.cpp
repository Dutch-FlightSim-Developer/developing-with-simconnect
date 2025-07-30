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

#include "pch.h"
#include <simconnect/data_frequency.hpp>

using namespace SimConnect;


TEST(TestPeriodLimits, DefaultConstructor) {
	PeriodLimits limits;
	EXPECT_EQ(limits.getOrigin(), 0u);
	EXPECT_EQ(limits.getLimit(), 0u);
}

TEST(TestPeriodLimits, ConstructorWithValues) {
	unsigned long origin = 5;
	unsigned long limit = 15;

	PeriodLimits limits(origin, limit);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, None) {
	PeriodLimits limits = PeriodLimits::none();
	EXPECT_EQ(limits.getOrigin(), 0u);
	EXPECT_EQ(limits.getLimit(), 0u);
}

TEST(TestPeriodLimits, StartAfter) {
	unsigned long origin = 5;

	PeriodLimits limits = PeriodLimits::startAfter(origin);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), 0u);
}

TEST(TestPeriodLimits, StopAfter) {
	unsigned long limit = 15;

	PeriodLimits limits = PeriodLimits::stopAfter(limit);
	EXPECT_EQ(limits.getOrigin(), 0u);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, AndStartAfter) {
	unsigned long origin = 5;
	unsigned long limit = 15;

	PeriodLimits limits = PeriodLimits::stopAfter(limit).andStartAfter(origin);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, AndStopAfter) {
	unsigned long origin = 5;
	unsigned long limit = 25;

	PeriodLimits limits = PeriodLimits::startAfter(origin).andStopAfter(limit);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}