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

#include <gtest/gtest.h>

#include "live_connection.hpp"

#include <simconnect/data_frequency.hpp>
#include <simconnect/requests/client_data_handler.hpp>

#include <cstdint>
#include <simconnect/simconnect.hpp>

#include <atomic>
#include <string_view>

using namespace SimConnect;


// The TwoFields struct intentionally uses mismatched native alignment and wire
// layout to make the typed-field path exercise padding-free serialization.
#pragma pack(push, 4)

struct TwoFields {
    int32_t field1{ 0 };
    double  field2{ 0.0 };
};

#pragma pack(pop)

// Datum IDs are assigned in addXxx() call order starting at 1.
// Used by the partial tagged send methods below.
//   kField1DatumId = 1
//   kField2DatumId = 2


// ---------------------------------------------------------------------------
// Helper classes
// ---------------------------------------------------------------------------

/**
 * The sender owns the client data area and writes to it.
 * Assumes the ClientDataHandler will gain send methods mirroring the request
 * methods already present.
 */
class TaggedSender : public LiveTests::LiveConnection {
public:
    ClientDataHandler<LiveTests::TestMessageHandler> clientDataHandler;
    MappedClientDataDefinition<TwoFields> def;
    ClientDataId dataId{ noClientDataId };

    explicit TaggedSender(std::string_view name)
        : LiveConnection(name), clientDataHandler(handler)
    {}

    bool setup(std::string_view areaName) {
        dataId = clientDataHandler.mapClientDataName(areaName);
        if (dataId == noClientDataId) { return false; }

        def.addInt32(&TwoFields::field1)
           .addFloat64(&TwoFields::field2);

        // Create the area using the wire size reported by the definition.
        if (!clientDataHandler.createClientData(dataId, def.size())) { return false; }

        def.define(connection);
        return connection.succeeded();
    }

    // Send the full struct as an untagged blob.
    void sendUntagged(const TwoFields& data) {
        clientDataHandler.sendClientData(dataId, def, data);
    }

    // Datum IDs are assigned in addXxx() call order starting at 1.
    static constexpr unsigned long kField1DatumId{ 1 };
    static constexpr unsigned long kField2DatumId{ 2 };

    // Send all fields in tagged (datum/value) format.
    void sendTaggedAll(const TwoFields& data) {
        clientDataHandler.sendClientDataTagged(dataId, def, data);
    }

    // Send only the fields whose datum IDs appear in the list, in tagged format.
    void sendTaggedField1Only(const TwoFields& data) {
        clientDataHandler.sendClientDataTagged(dataId, def, data, { kField1DatumId });
    }

    void sendTaggedField2Only(const TwoFields& data) {
        clientDataHandler.sendClientDataTagged(dataId, def, data, { kField2DatumId });
    }
};


/**
 * The receiver maps the same area name (different local ID, same SimConnect
 * area) and subscribes with varying flag combinations.
 */
class TaggedReceiver : public LiveTests::LiveConnection {
public:
    ClientDataHandler<LiveTests::TestMessageHandler> clientDataHandler;
    MappedClientDataDefinition<TwoFields> def;
    ClientDataId dataId{ noClientDataId };

    explicit TaggedReceiver(std::string_view name)
        : LiveConnection(name), clientDataHandler(handler)
    {}

    bool setup(std::string_view areaName) {
        dataId = clientDataHandler.mapClientDataName(areaName);
        if (dataId == noClientDataId) { return false; }

        def.addInt32(&TwoFields::field1)
           .addFloat64(&TwoFields::field2);

        return connection.succeeded();
    }
};


// ---------------------------------------------------------------------------
// Tests
//
// Send-side tagging and receive-side tagging are independent.  The data area
// is the shared intermediary: one side writes (as raw blob OR datum/value
// pairs) and the other side reads back (as raw blob OR datum/value pairs)
// without any coupling between the two choices.
//
// The four tests each fix the receive mode and exercise all three send modes:
//   1. Untagged receive              — all send modes
//   2. Untagged + when-changed       — all send modes
//   3. Tagged receive                — all send modes
//   4. Tagged + when-changed         — all send modes
// ---------------------------------------------------------------------------

//NOLINTBEGIN(readability-function-cognitive-complexity)

/**
 * Untagged receive, no when-changed.
 *
 * Every write — regardless of whether it was sent tagged or untagged — must
 * arrive at the receiver, and both fields must round-trip correctly.
 */
TEST(TestTaggedClientData, UntaggedReceive) {
    TaggedSender   sender  ("TaggedCDA_Sender_UR");
    TaggedReceiver receiver("TaggedCDA_Receiver_UR");

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());
    ASSERT_TRUE(sender.setup  ("CppSimConnect.Test.TaggedClientData.UR"));
    ASSERT_TRUE(receiver.setup("CppSimConnect.Test.TaggedClientData.UR"));

    std::atomic<int> receiveCount{ 0 };
    TwoFields received{};

    receiver.def.define(receiver.connection);
    auto request = receiver.clientDataHandler.requestClientData(
        receiver.dataId, receiver.def,
        [&](const TwoFields& data) {
            received = data;
            receiveCount++;
        },
        ClientDataFrequency::onSet());
    auto msgLogger = receiver.clientDataHandler.requestClientData(
        receiver.dataId, receiver.def,
        [&](const Messages::ClientDataMsg& msg) {
            // Log the raw message for debugging.
            std::string logMsg = "Received ClientDataMsg: dwRequestID=" + std::to_string(msg.dwRequestID) +
                                 ", dwDefineID=" + std::to_string(msg.dwDefineID) +
                                 ", dwDefineCount=" + std::to_string(msg.dwDefineCount) +
                                 ", dwSize=" + std::to_string(msg.dwSize) +
                                 ", flags=" + std::to_string(msg.dwFlags) +
                                 ", data size=" + std::to_string(msg.dwSize - sizeof(Messages::ClientDataMsg) - sizeof(DWORD));
            receiver.connection.logger().debug(logMsg);
        },
        ClientDataFrequency::onSet());

    // Send mode 1: untagged full blob.
    const TwoFields data1{ .field1 = 1, .field2 = 1.0 };
    sender.sendUntagged(data1);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 1; }));
    EXPECT_EQ       (received.field1,  1);
    EXPECT_DOUBLE_EQ(received.field2,  1.0);

    // Send mode 2: tagged, all fields.
    const TwoFields data2{ .field1 = 2, .field2 = 2.0 };
    sender.sendTaggedAll(data2);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 2; }));
    EXPECT_EQ       (received.field1,  2);
    EXPECT_DOUBLE_EQ(received.field2,  2.0);

    // Send mode 3a: tagged, only field1 changed.
    const TwoFields data3{ .field1 = 3, .field2 = 2.0 };
    sender.sendTaggedField1Only(data3);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 3; }));
    EXPECT_EQ(received.field1, 3);

    // Send mode 3b: tagged, only field2 changed.
    const TwoFields data4{ .field1 = 3, .field2 = 4.0 };
    sender.sendTaggedField2Only(data4);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 4; }));
    EXPECT_DOUBLE_EQ(received.field2, 4.0);

    sender.close();
    receiver.close();
}


/**
 * Untagged receive with when-changed.
 *
 * The when-changed flag is purely a receive-side concept: SimConnect compares
 * the current area contents to what it last delivered and skips the update if
 * nothing changed.  This must work regardless of how the sender wrote the data.
 */
TEST(TestTaggedClientData, UntaggedWhenChangedReceive) {
    TaggedSender   sender  ("TaggedCDA_Sender_UWCR");
    TaggedReceiver receiver("TaggedCDA_Receiver_UWCR");

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());
    ASSERT_TRUE(sender.setup  ("CppSimConnect.Test.TaggedClientData.UWCR"));
    ASSERT_TRUE(receiver.setup("CppSimConnect.Test.TaggedClientData.UWCR"));

    std::atomic<int> receiveCount{ 0 };
    TwoFields received{};

    auto request = receiver.clientDataHandler.requestClientData(
        receiver.dataId, receiver.def,
        [&](const TwoFields& data) {
            received = data;
            receiveCount++;
        },
        ClientDataFrequency::onSet(),
        PeriodLimits::none(),
        true /*onlyWhenChanged*/);

    // Send mode 1: untagged.
    const TwoFields data1{ .field1 = 10, .field2 = 10.0 };
    sender.sendUntagged(data1);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 1; }));

    sender.sendUntagged(data1);                        // same data — must be suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 1) << "Unchanged untagged send must be suppressed";

    const TwoFields data2{ .field1 = 20, .field2 = 20.0 };
    sender.sendUntagged(data2);                        // changed data
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 2; }));
    EXPECT_EQ       (received.field1,  20);
    EXPECT_DOUBLE_EQ(received.field2,  20.0);

    // Send mode 2: tagged, all fields.
    sender.sendTaggedAll(data2);                       // same as last delivered — suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 2) << "Unchanged tagged-all send must be suppressed";

    const TwoFields data3{ .field1 = 30, .field2 = 30.0 };
    sender.sendTaggedAll(data3);                       // changed — delivered
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 3; }));
    EXPECT_EQ(received.field1, 30);

    // Send mode 3: tagged, partial.
    sender.sendTaggedField1Only(data3);                // field1 unchanged — suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 3) << "Partial tagged send with no net change must be suppressed";

    const TwoFields data4{ .field1 = 40, .field2 = 30.0 };
    sender.sendTaggedField1Only(data4);                // field1 changed — delivered
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 4; }));
    EXPECT_EQ(received.field1, 40);

    sender.close();
    receiver.close();
}


/**
 * Tagged receive, no when-changed.
 *
 * The receive-side TAGGED flag tells SimConnect to deliver datum/value pairs
 * instead of a raw blob.  This is independent of what the sender chose: the
 * receiver must correctly reconstruct TwoFields from the tagged packets
 * regardless of whether the sender wrote an untagged blob, a fully tagged
 * packet, or a partial tagged packet.
 */
TEST(TestTaggedClientData, TaggedReceive) {
    TaggedSender   sender  ("TaggedCDA_Sender_TR");
    TaggedReceiver receiver("TaggedCDA_Receiver_TR");

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());
    ASSERT_TRUE(sender.setup  ("CppSimConnect.Test.TaggedClientData.TR"));
    ASSERT_TRUE(receiver.setup("CppSimConnect.Test.TaggedClientData.TR"));

    std::atomic<int> receiveCount{ 0 };
    TwoFields received{};

    auto request = receiver.clientDataHandler.requestClientDataTagged(
        receiver.dataId, receiver.def,
        [&](const TwoFields& data) {
            received = data;
            receiveCount++;
        },
        ClientDataFrequency::onSet());

    // Send mode 1: untagged blob — receiver still gets tagged datum/value pairs.
    const TwoFields data1{ .field1 = 1, .field2 = 1.0 };
    sender.sendUntagged(data1);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 1; }));
    EXPECT_EQ       (received.field1,  1);
    EXPECT_DOUBLE_EQ(received.field2,  1.0);

    // Send mode 2: tagged, all fields.
    const TwoFields data2{ .field1 = 2, .field2 = 2.0 };
    sender.sendTaggedAll(data2);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 2; }));
    EXPECT_EQ       (received.field1,  2);
    EXPECT_DOUBLE_EQ(received.field2,  2.0);

    sender.sendTaggedAll(data2);                       // same data, no when-changed — must arrive
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 3; }));

    // Send mode 3a: tagged, only field1 changed.
    const TwoFields data3{ .field1 = 3, .field2 = 2.0 };
    sender.sendTaggedField1Only(data3);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 4; }));
    EXPECT_EQ(received.field1, 3);

    // Send mode 3b: tagged, only field2 changed.
    const TwoFields data4{ .field1 = 3, .field2 = 4.0 };
    sender.sendTaggedField2Only(data4);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 5; }));
    EXPECT_DOUBLE_EQ(received.field2, 4.0);

    sender.close();
    receiver.close();
}


/**
 * Tagged receive with when-changed.
 *
 * Combines the tagged receive path with when-changed suppression.  Suppression
 * is based on whether the area contents actually changed, regardless of which
 * send mode wrote the data.
 */
TEST(TestTaggedClientData, TaggedWhenChangedReceive) {
    TaggedSender   sender  ("TaggedCDA_Sender_TWCR");
    TaggedReceiver receiver("TaggedCDA_Receiver_TWCR");

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());
    ASSERT_TRUE(sender.setup  ("CppSimConnect.Test.TaggedClientData.TWCR"));
    ASSERT_TRUE(receiver.setup("CppSimConnect.Test.TaggedClientData.TWCR"));

    std::atomic<int> receiveCount{ 0 };
    TwoFields received{};

    auto request = receiver.clientDataHandler.requestClientDataTagged(
        receiver.dataId, receiver.def,
        [&](const TwoFields& data) {
            received = data;
            receiveCount++;
        },
        ClientDataFrequency::onSet(),
        PeriodLimits::none(),
        true /*onlyWhenChanged*/);

    // Send mode 1: untagged.
    const TwoFields data1{ .field1 = 10, .field2 = 10.0 };
    sender.sendUntagged(data1);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 1; }));
    EXPECT_EQ       (received.field1,  10);
    EXPECT_DOUBLE_EQ(received.field2,  10.0);

    sender.sendUntagged(data1);                        // same — suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 1) << "Unchanged untagged send must be suppressed";

    // Send mode 2: tagged, all fields.
    const TwoFields data2{ .field1 = 20, .field2 = 20.0 };
    sender.sendTaggedAll(data2);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 2; }));

    sender.sendTaggedAll(data2);                       // same — suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 2) << "Unchanged tagged-all send must be suppressed";

    // Send mode 3: tagged, partial — only field2 changes.
    const TwoFields data3{ .field1 = 20, .field2 = 99.9 };
    sender.sendTaggedField2Only(data3);
    EXPECT_TRUE(receiver.waitUntil([&] { return receiveCount.load() >= 3; }));
    EXPECT_DOUBLE_EQ(received.field2, 99.9);

    sender.sendTaggedField2Only(data3);                // same field2 again — suppressed
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_EQ(receiveCount.load(), 3) << "Partial tagged send with no net change must be suppressed";

    sender.close();
    receiver.close();
}

//NOLINTEND(readability-function-cognitive-complexity)
