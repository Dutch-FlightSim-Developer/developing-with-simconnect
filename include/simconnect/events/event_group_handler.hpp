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


#include <map>
#include <tuple>
#include <functional>

#include <simconnect/simconnect.hpp>

#include <simconnect/message_handler.hpp>
#include <simconnect/events/event_group.hpp>


namespace SimConnect {


/**
 * The EventHandler class provides base dispatching functionality for Events.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 */
template <class M>
class EventGroupHandler : public MessageHandler<EventGroupId, EventGroupHandler<M>, M,
    Messages::event,
    Messages::eventEx1,
    Messages::eventObjectAddRemove,
    Messages::eventFilename,
    Messages::eventFrame,
    Messages::eventWeatherMode,
    Messages::eventMultiplayerServerStarted,
    Messages::eventMultiplayerClientStarted,
    Messages::eventMultiplayerSessionEnded,
    Messages::eventRaceLap,
    Messages::eventRaceEnd>
{
	using Base = MessageHandler<EventGroupId, EventGroupHandler<M>, M,
        Messages::event,
        Messages::eventEx1,
        Messages::eventObjectAddRemove,
        Messages::eventFilename,
        Messages::eventFrame,
        Messages::eventWeatherMode,
        Messages::eventMultiplayerServerStarted,
        Messages::eventMultiplayerClientStarted,
        Messages::eventMultiplayerSessionEnded,
        Messages::eventRaceLap,
        Messages::eventRaceEnd>;


public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;


protected:
    simconnect_message_handler_type& simConnectMessageHandler_;


public:
    inline connection_type& connection() {
        return simConnectMessageHandler_.connection();
    }
    inline logger_type& logger() {
        return simConnectMessageHandler_.logger();
    }


private:
    // No copies or moves
    EventGroupHandler(const EventGroupHandler&) = delete;
    EventGroupHandler(EventGroupHandler&&) = delete;
    EventGroupHandler& operator=(const EventGroupHandler&) = delete;
    EventGroupHandler& operator=(EventGroupHandler&&) = delete;


public:
    EventGroupHandler(simconnect_message_handler_type& handler) : Base(), simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~EventGroupHandler() = default;

    /**
     * Returns the correlation ID from the message. For event messages, this is the event ID.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const Messages::MsgBase& msg) {
        // All event message types have a uGroupID
        return static_cast<const Messages::EventMsg&>(msg).uGroupID;
    }


#pragma region EventGroup Handling

    /**
     * Register a handler for a specific group ID with typed event message.
     * 
     * @tparam EventType The specific event message type (Messages::EventMsg, etc.)
     * @param groupId The group ID to register.
     * @param handler The typed handler to call when the event is received.
     * @param autoRemove True to automatically remove the handler after it has been called.
     * @returns A reference to this EventHandler.
     */
    template <typename EventType = Messages::EventMsg>
    EventGroupHandler& registerGroupHandler(EventGroupId groupId, 
                             std::function<void(const EventType&)> handler,
                             bool autoRemove = false) {
        this->registerHandler(groupId, [handler](const Messages::MsgBase& msg) {
            handler(reinterpret_cast<const EventType&>(msg));
        }, autoRemove);

        return *this;
    }


    /**
     * Remove a handler for a specific group ID.
     * 
     * @param groupId The group ID to remove the handler for.
     * @returns A reference to this EventHandler.
     */
    EventGroupHandler& removeGroupHandler(EventGroupId groupId) {
        this->removeHandler(groupId);
        return *this;
    }

#pragma endregion

};

} // namespace SimConnect