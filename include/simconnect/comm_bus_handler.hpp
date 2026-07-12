#pragma once
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

#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>
#include <simconnect/requests/request.hpp>


namespace SimConnect {


#if MSFS_2024_SDK

/**
 * The CommBusHandler class provides for responsive handling of Messages::CommBusMsg messages,
 * reassembling multi-part CommBus broadcasts into a single payload before delivery.
 *
 * @note CommBus event IDs (SimConnect::CommBusEventId) occupy their own ID namespace, separate
 * from regular client event IDs (SimConnect::EventId) - confirmed by testing: subscribing a
 * CommBus event and mapping a client event to a sim event with the same numeric ID produces no
 * SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE, regardless of registration order. IDs handed out by
 * this handler are therefore drawn from their own counter.
 *
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 */
template <class M>
class CommBusHandler
    : public MessageHandler<CommBusEventId, CommBusHandler<M>, M, Messages::commBus>
{
    using Base = MessageHandler<CommBusEventId, CommBusHandler<M>, M, Messages::commBus>;

public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using mutex_type = typename connection_type::mutex_type;
    using guard_type = typename connection_type::guard_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;

    mutex_type buffersMutex_;
    std::unordered_map<CommBusEventId, std::string> buffers_; ///< In-progress reassembly buffers, keyed by CommBusEventId.


    // No copies or moves
    CommBusHandler(const CommBusHandler&) = delete;
    CommBusHandler(CommBusHandler&&) = delete;
    CommBusHandler& operator=(const CommBusHandler&) = delete;
    CommBusHandler& operator=(CommBusHandler&&) = delete;


    /**
     * Allocates the next CommBusEventId. Drawn from its own counter, independent of the
     * Connection's client-event ID allocator (see class note on separate ID namespaces).
     */
    [[nodiscard]]
    static CommBusEventId nextCommBusEventId() {
        static std::atomic<CommBusEventId> nextId{ 1 };
        return nextId++;
    }


public:
    CommBusHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~CommBusHandler() = default;


    /**
     * Returns the SimConnect message handler.
     */
    [[nodiscard]]
    simconnect_message_handler_type& simConnectMessageHandler() const noexcept {
        return simConnectMessageHandler_;
    }


    /**
     * Returns the correlation ID from the message. For CommBus messages, this is the
     * CommBusEventId passed to subscribeToEvent().
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const Messages::MsgBase& msg) {
        return static_cast<const Messages::CommBusMsg&>(msg).uEventID;
    }


    /**
     * Subscribes to a CommBus event, reassembling multi-part broadcasts into a single payload
     * before delivery. Single-chunk broadcasts (the common case) are delivered directly, without
     * ever touching the reassembly buffer.
     *
     * @param eventName The CommBus event name to subscribe to.
     * @param handler The handler to invoke with the fully reassembled payload.
     * @return A Request that unsubscribes when destroyed or stopped.
     */
    [[nodiscard]]
    Request subscribeToEvent(std::string_view eventName, std::function<void(std::string_view)> handler) {
        const auto id = nextCommBusEventId();

        this->registerHandler(id, [this, id, handler](const Messages::MsgBase& msg) {
            const auto& commBusMsg = static_cast<const Messages::CommBusMsg&>(msg);

            if (commBusMsg.dwOutOf == 1) {
                handler(std::string_view{ commBusMsg.rgData });
                return;
            }

            // Assemble under lock, but invoke the caller's handler after releasing it -
            // matches MessageHandler::dispatch's own discipline of never calling out while locked.
            bool complete{ false };
            std::string payload;
            {
                guard_type lock(buffersMutex_);
                auto& buffer = buffers_[id];
                if (commBusMsg.dwEntryNumber == 0) {
                    buffer.clear();
                }
                buffer.append(commBusMsg.rgData);

                complete = (commBusMsg.dwEntryNumber + 1 == commBusMsg.dwOutOf);
                if (complete) {
                    payload = std::move(buffer);
                    buffers_.erase(id);
                }
            }
            if (complete) {
                handler(std::string_view{ payload });
            }
        }, false);

        simConnectMessageHandler_.connection().subscribeToCommBusEvent(id, eventName);

        return Request{ id, [this, id]() {
            this->removeHandler(id);
            {
                guard_type lock(buffersMutex_);
                buffers_.erase(id);
            }
            simConnectMessageHandler_.connection().unsubscribeFromCommBusEvent(id);
        }};
    }


    /**
     * Broadcasts a CommBus event with a string payload.
     *
     * @param eventName The CommBus event name to broadcast.
     * @param data The payload to send.
     * @param broadcastTo The target platform(s) to broadcast the event to. Defaults to CommBusBroadcastTo::defaultFlag (JS + WASM + other SimConnect clients).
     */
    void sendEvent(std::string_view eventName, std::string_view data,
        CommBusBroadcastToFlag broadcastTo = CommBusBroadcastTo::defaultFlag) {
        simConnectMessageHandler_.connection().callCommBusEvent(eventName, data, broadcastTo);
    }

};

#else
#error "comm_bus_handler.hpp requires the MSFS 2024 SDK (CommBus is not available in earlier SDK versions)."
#endif // MSFS_2024_SDK

} // namespace SimConnect
