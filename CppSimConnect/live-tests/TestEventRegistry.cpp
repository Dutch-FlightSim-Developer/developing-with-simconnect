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
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include <gtest/gtest.h>

#include "live_connection.hpp"

#include <simconnect/simconnect.hpp>
#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <chrono>

using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)

// event() registers name; eventName() retrieves it; unknown ID returns empty
TEST(TestEventRegistry, EventNameLookup) {
    LiveTests::LiveConnection lc("EventNameLookupTest");
    ASSERT_TRUE(lc.openAndWait());

    auto evt = lc.connection.event("Registry.TestEvent");

    EXPECT_EQ(lc.connection.eventName(evt.id()), "Registry.TestEvent");
    EXPECT_EQ(lc.connection.eventName(evt.id() + 99999u), "");

    lc.close();
}

// Registry is cleared on close; same name gets a new (higher) ID on reopen
TEST(TestEventRegistry, RegistryClearedOnClose) {
    LiveTests::LiveConnection lc("RegistryClearTest");
    ASSERT_TRUE(lc.openAndWait());

    auto evt = lc.connection.event("Registry.ClearTest");
    auto firstId = evt.id();

    EXPECT_EQ(lc.connection.eventName(firstId), "Registry.ClearTest");

    lc.close();

    EXPECT_EQ(lc.connection.eventName(firstId), "") << "Registry must be cleared on close";

    ASSERT_TRUE(lc.open());

    auto evt2 = lc.connection.event("Registry.ClearTest");

    EXPECT_NE(evt2.id(), firstId) << "Reopened connection must allocate new ID";
    EXPECT_GT(evt2.id(), firstId) << "Global ID counter must not reset";
    EXPECT_EQ(lc.connection.eventName(evt2.id()), "Registry.ClearTest");
    EXPECT_EQ(lc.connection.eventName(firstId), "") << "Old ID must remain unknown after reopen";

    lc.close();
}

// Two connections registering the same name get distinct IDs; each registry is isolated
TEST(TestEventRegistry, CrossConnectionIdsAreDistinct) {
    LiveTests::LiveConnection lc1("CrossConnTest1");
    LiveTests::LiveConnection lc2("CrossConnTest2");

    ASSERT_TRUE(lc1.openAndWait());
    ASSERT_TRUE(lc2.openAndWait());

    auto evt1 = lc1.connection.event("CrossConn.SharedName");
    auto evt2 = lc2.connection.event("CrossConn.SharedName");

    EXPECT_NE(evt1.id(), evt2.id()) << "Each connection must get its own ID";

    EXPECT_EQ(lc1.connection.eventName(evt1.id()), "CrossConn.SharedName");
    EXPECT_EQ(lc2.connection.eventName(evt2.id()), "CrossConn.SharedName");

    EXPECT_EQ(lc1.connection.eventName(evt2.id()), "") << "lc1 must not know lc2's event ID";
    EXPECT_EQ(lc2.connection.eventName(evt1.id()), "") << "lc2 must not know lc1's event ID";

    lc1.close();
    lc2.close();
}

// TODO: TrackMappedEvents=false test (double-map reaches SimConnect without exception)
// Blocked by WindowsEventHandler hardcoding TrackMappedEvents=true in its connection_type.
// Needs WindowsEventHandler to forward the TrackMappedEvents parameter.

//NOLINTEND(readability-function-cognitive-complexity)
