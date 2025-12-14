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

#include "gtest/gtest.h"

#include <simconnect/data_frequency.hpp>

using namespace SimConnect;


TEST(TestPeriodLimits, DefaultConstructor) {
	const PeriodLimits limits;
	EXPECT_EQ(limits.getOrigin(), 0U);
	EXPECT_EQ(limits.getLimit(), 0U);
}

TEST(TestPeriodLimits, ConstructorWithValues) {
	const unsigned long origin = 5;
	const unsigned long limit = 15;

	const PeriodLimits limits(origin, limit);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, None) {
	const PeriodLimits limits = PeriodLimits::none();
	EXPECT_EQ(limits.getOrigin(), 0U);
	EXPECT_EQ(limits.getLimit(), 0U);
}

TEST(TestPeriodLimits, StartAfter) {
	const unsigned long origin = 5;

	const PeriodLimits limits = PeriodLimits::startAfter(origin);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), 0U);
}

TEST(TestPeriodLimits, StopAfter) {
	const unsigned long limit = 15;
	const PeriodLimits limits = PeriodLimits::stopAfter(limit);
	EXPECT_EQ(limits.getOrigin(), 0U);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, AndStartAfter) {
	const unsigned long origin = 5;
	const unsigned long limit = 15;

	const PeriodLimits limits = PeriodLimits::stopAfter(limit).andStartAfter(origin);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}

TEST(TestPeriodLimits, AndStopAfter) {
	const unsigned long origin = 5;
	const unsigned long limit = 25;

	const PeriodLimits limits = PeriodLimits::startAfter(origin).andStopAfter(limit);
	EXPECT_EQ(limits.getOrigin(), origin);
	EXPECT_EQ(limits.getLimit(), limit);
}