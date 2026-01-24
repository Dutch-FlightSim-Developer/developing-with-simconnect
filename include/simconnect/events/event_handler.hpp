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
#include <type_traits>

#include <simconnect/simconnect.hpp>

#include <simconnect/message_handler.hpp>
#include <simconnect/messaging/handler_policy.hpp>
#include <simconnect/events/events.hpp>
#include <simconnect/events/event_group.hpp>
#include <simconnect/events/event_group_handler.hpp>


namespace SimConnect {

// Forward declarations to break circular dependency
template <class M, bool EnableEventGroupHandler = true>
class NotificationGroup;

template <class M, bool EnableEventGroupHandler = true>
class InputGroup;


template <class M>
class NoGroupHandler {
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;


    NoGroupHandler([[maybe_unused]] simconnect_message_handler_type& simConnectMessageHandler) {}
    ~NoGroupHandler() = default;

    template <typename EventType = Messages::EventMsg>
    inline void registerGroupHandler([[maybe_unused]] EventGroupId groupId, 
                             [[maybe_unused]] std::function<void(const EventType&)> handler,
                             [[maybe_unused]] bool autoRemove = false) {
    }


    inline void removeGroupHandler([[maybe_unused]] EventGroupId groupId) {
    }
};


/**
 * The EventHandler class provides base dispatching functionality for Events.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 */
template <class M, bool EnableEventGroupHandler = true>
class EventHandler : public MessageHandler<EventId, EventHandler<M>, M,
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
	using Base = MessageHandler<EventId, EventHandler<M>, M,
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

    using event_group_handler_type = std::conditional_t<EnableEventGroupHandler, EventGroupHandler<M>, NoGroupHandler<M>>;


protected:
    simconnect_message_handler_type& simConnectMessageHandler_;
    event_group_handler_type eventGroupHandler_;

public:
    inline connection_type& connection() {
        return simConnectMessageHandler_.connection();
    }
    inline logger_type& logger() {
        return simConnectMessageHandler_.logger();
    }


private:
    // No copies or moves
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;


public:
    EventHandler(simconnect_message_handler_type& handler) : Base(), simConnectMessageHandler_(handler), eventGroupHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~EventHandler() = default;


    /**
     * Returns the correlation ID from the message. For event messages, this is the event ID.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const Messages::MsgBase& msg) {
        // All event message types have uEventID as the first field after the base Messages::MsgBase
        return static_cast<const Messages::EventMsg&>(msg).uEventID;
    }


#pragma region Notification Group Factory

    /**
     * Create a new notification group.
     * 
     * @returns A new NotificationGroup instance associated with this EventHandler.
     */
    [[nodiscard]]
    NotificationGroup<M, EnableEventGroupHandler> createNotificationGroup();

#pragma endregion

#pragma region Input Group Factory

    /**
     * Create a new input group.
     * 
     * @returns A new InputGroup instance associated with this EventHandler.
     */
    [[nodiscard]]
    InputGroup<M, EnableEventGroupHandler> createInputGroup();

#pragma endregion

#pragma region Event Mapping

    /**
     * Maps a client event to a simulator event.
     * The event will be mapped using its own name.
     * 
     * @param evt The event to map.
     * @returns A reference to this EventHandler.
     */
    EventHandler& mapEvent(event evt) {
        connection().mapClientEvent(evt);
        return *this;
    }

#pragma endregion

#pragma region Event Handling

    /**
     * Register a handler for a specific event ID with typed event message.
     * 
     * @tparam EventType The specific event message type (Messages::EventMsg, etc.)
     * @param eventId The event ID to register.
     * @param handler The typed handler to call when the event is received.
     * @param autoRemove True to automatically remove the handler after it has been called.
     * @returns A reference to this EventHandler.
     */
    template <typename EventType = Messages::EventMsg>
    EventHandler& registerEventHandler(EventId eventId, 
                             std::function<void(const EventType&)> handler,
                             bool autoRemove = false) {
        this->logger().debug(std::format("Registering handler for event ID {} (autoremove={})", eventId, autoRemove));
        this->registerHandler(eventId, [handler](const Messages::MsgBase& msg) {
            handler(reinterpret_cast<const EventType&>(msg));
        }, autoRemove);

        return *this;
    }


    /**
     * Remove a handler for a specific event ID.
     * 
     * @param eventId The event ID to remove the handler for.
     * @returns A reference to this EventHandler.
     */
    EventHandler& removeEventHandler(EventId eventId) {
        this->logger().debug(std::format("Removing handler for event ID {}", eventId));
        this->removeHandler(eventId);
        return *this;
    }


    /**
     * Register a handler for all Evebts in a specific Notification- or Input Group.
     * 
     * @tparam EventType The specific event message type (Messages::EventMsg, etc.)
     * @param groupId The group ID to register.
     * @param handler The typed handler to call when an event in the group is received.
     * @param autoRemove True to automatically remove the handler after it has been called.
     * @returns A reference to this EventHandler.
     */
    template <typename EventType = Messages::EventMsg>
    EventHandler& registerEventGroupHandler(EventGroupId groupId, 
                             std::function<void(const EventType&)> handler,
                             bool autoRemove = false) {
        this->logger().debug(std::format("Registering group handler for event group ID {} (autoremove={})", groupId, autoRemove));
        eventGroupHandler_.template registerGroupHandler<EventType>(groupId, handler, autoRemove);
        return *this;
    }


    /**
     * Remove a handler for all Events in a specific Notification- or Input Group.
     * 
     * @param groupId The group ID to remove the handler for.
     * @returns A reference to this EventHandler.
     */
    EventHandler& removeEventGroupHandler(EventGroupId groupId) {
        this->logger().debug(std::format("Removing group handler for event group ID {}", groupId));
        eventGroupHandler_.removeGroupHandler(groupId);
        return *this;
    }

#pragma endregion

#pragma region Event Sending

    /**
     * Send an Event.
     * 
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data The optional data to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEvent(event evt, NotificationGroupId groupId, unsigned long data = 0) {
        this->connection().transmitClientEvent(SimObject::user, evt, groupId, data);
        return *this;
    }


    /**
     * Send an Event, not in a group, but with a priority.
     * 
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data The optional data to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventWithPriority(event evt, unsigned long priority, unsigned long data = 0) {
        this->connection().transmitClientEventWithPriority(SimObject::user, evt, priority, data);
        return *this;
    }


    /**
     * Send an Event to a specific object.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data The optional data to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventToObject(SimObjectId objectId, event evt, NotificationGroupId groupId, unsigned long data = 0) {
        this->connection().transmitClientEvent(objectId, evt, groupId, data);
        return *this;
    }


    /**
     * Send an Event to a specific object, not in a group, but with a priority.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data The optional data to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventToObjectWithPriority(SimObjectId objectId, event evt, unsigned long priority, unsigned long data = 0) {
        this->connection().transmitClientEventWithPriority(objectId, evt, priority, data);
        return *this;
    }


    /**
     * Send an Event with more data.
     * 
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEvent(event evt, NotificationGroupId groupId, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEvent(SimObject::user, evt, groupId, data0, data1, data2, data3, data4);
        return *this;
    }


    /**
     * Send an Event with more data, not in a group, but with a priority.
     * 
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventWithPriority(event evt, unsigned long priority, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEventWithPriority(SimObject::user, evt, priority, data0, data1, data2, data3, data4);
        return *this;
    }


    /**
     * Send an Event to a specific object, with more data.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventToObject(SimObjectId objectId, event evt, NotificationGroupId groupId, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEvent(objectId, evt, groupId, data0, data1, data2, data3, data4);
        return *this;
    }


    /**
     * Send an Event to a specific object, with more data, not in a group, but with a priority.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     * @returns A reference to this EventHandler.
     */
    EventHandler& sendEventToObjectWithPriority(SimObjectId objectId, event evt, unsigned long priority, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEventWithPriority(objectId, evt, priority, data0, data1, data2, data3, data4);
        return *this;
    }

#pragma endregion

};

} // namespace SimConnect