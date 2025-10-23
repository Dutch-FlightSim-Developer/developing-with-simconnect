#pragma once
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

#include <functional>

#include <simconnect/message_handler.hpp>
#include <simconnect/events/events.hpp>


namespace SimConnect {


using BaseEventHandler = std::function<void(const SIMCONNECT_RECV_EVENT&)>;


/**
 * The SystemEventHandler class provides for responsive handling of system events using correlation IDs.
 * 
 * This handler can process:
 * - Plain event messages (SIMCONNECT_RECV_EVENT)
 * - Filename event messages (SIMCONNECT_RECV_EVENT_FILENAME) 
 * - Object add/remove messages (SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE)
 */
template <class M>
class SystemEventHandler : public MessageHandler<DWORD, SystemEventHandler<M>, M,
                                                 SIMCONNECT_RECV_ID_EVENT, 
                                                 SIMCONNECT_RECV_ID_EVENT_FILENAME,
                                                 SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    SystemEventHandler(const SystemEventHandler&) = delete;
    SystemEventHandler(SystemEventHandler&&) = delete;
    SystemEventHandler& operator=(const SystemEventHandler&) = delete;
    SystemEventHandler& operator=(SystemEventHandler&&) = delete;

public:
    SystemEventHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~SystemEventHandler() = default;


    /**
     * Returns the correlation ID from the message. For event messages, this is the event ID.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const SIMCONNECT_RECV& msg) const {
        // All event message types have uEventID as the first field after the base SIMCONNECT_RECV
        return reinterpret_cast<const SIMCONNECT_RECV_EVENT&>(msg).uEventID;
    }


    /**
     * Subscribe to a system event.
     * 
     * @param connection The connection to subscribe with.
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received.
     */
    void subscribeToSystemEvent(connection_type& connection, event systemStateEvent, BaseEventHandler handler) {
        this->registerHandler(systemStateEvent, [handler](const SIMCONNECT_RECV& msg) {
            handler(reinterpret_cast<const SIMCONNECT_RECV_EVENT&>(msg));
        }, false);
        simConnectMessageHandler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Unsubscribe from a system event.
     * 
     * @param connection The connection to unsubscribe from.
     * @param systemStateEvent The event to unsubscribe from.
     */
    void unsubscribeFromSystemEvent(connection_type& connection, event systemStateEvent) {
        simConnectMessageHandler_.connection().unsubscribeFromSystemEvent(systemStateEvent);
        this->removeHandler(systemStateEvent);
    }


    /**
     * Subscribe to a system event with a handler that receives the specific event message type.
     * 
     * @tparam EventType The specific event message type (SIMCONNECT_RECV_EVENT, etc.)
     * @param connection The connection to subscribe with.
     * @param systemStateEvent The event to subscribe to.
     * @param handler The typed handler to call when the event is received.
     */
    template <typename EventType>
    void subscribeToSystemEvent(connection_type& connection, event systemStateEvent, 
                               std::function<void(const EventType&)> handler) {
        this->registerHandler(systemStateEvent, [handler](const SIMCONNECT_RECV& msg, [[maybe_unused]] DWORD size) {
            handler(*reinterpret_cast<const EventType*>(msg));
        }, false);
        simConnectMessageHandler_.connection().subscribeToSystemEvent(systemStateEvent);
    }
};

} // namespace SimConnect
