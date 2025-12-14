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


#include <simconnect/message_handler.hpp>
#include <simconnect/events/events.hpp>


namespace SimConnect {

// Forward declaration to break circular dependency
template <class M>
class NotificationGroup;


/**
 * The EventHandler class provides base dispatching functionality for Events.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 * @tparam Derived The derived class type (CRTP pattern).
 * @tparam id The SIMCONNECT_RECV_IDs that this handler will respond to.
 */
template <class M>
class EventHandler : public MessageHandler<DWORD, EventHandler<M>, M,
    SIMCONNECT_RECV_ID_EVENT,
    SIMCONNECT_RECV_ID_EVENT_EX1,
    SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE,
    SIMCONNECT_RECV_ID_EVENT_FILENAME,
    SIMCONNECT_RECV_ID_EVENT_FRAME,
    SIMCONNECT_RECV_ID_EVENT_WEATHER_MODE,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED,
    SIMCONNECT_RECV_ID_EVENT_RACE_LAP,
    SIMCONNECT_RECV_ID_EVENT_RACE_END>
{
	using Base = MessageHandler<DWORD, EventHandler<M>, M,
        SIMCONNECT_RECV_ID_EVENT,
        SIMCONNECT_RECV_ID_EVENT_EX1,
        SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE,
        SIMCONNECT_RECV_ID_EVENT_FILENAME,
        SIMCONNECT_RECV_ID_EVENT_FRAME,
        SIMCONNECT_RECV_ID_EVENT_WEATHER_MODE,
        SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED,
        SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED,
        SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED,
        SIMCONNECT_RECV_ID_EVENT_RACE_LAP,
        SIMCONNECT_RECV_ID_EVENT_RACE_END>;


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
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;


public:
    EventHandler(simconnect_message_handler_type& handler) : Base(), simConnectMessageHandler_(handler)
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
    unsigned long correlationId(const SIMCONNECT_RECV& msg) const {
        // All event message types have uEventID as the first field after the base SIMCONNECT_RECV
        return static_cast<const SIMCONNECT_RECV_EVENT&>(msg).uEventID;
    }


#pragma region Notification Group Factory

    /**
     * Create a new notification group.
     * 
     * @returns A new NotificationGroup instance associated with this EventHandler.
     */
    [[nodiscard]]
    NotificationGroup<M> createNotificationGroup();

#pragma endregion

#pragma region Event Mapping

    /**
     * Maps a client event to a simulator event.
     * The event will be mapped using its own name.
     * 
     * @param evt The event to map.
     */
    void mapEvent(event evt) {
        connection().mapClientEvent(evt);
    }

#pragma endregion

#pragma region Event Handling

    /**
     * Register a handler for a specific event ID with typed event message.
     * 
     * @tparam EventType The specific event message type (SIMCONNECT_RECV_EVENT, etc.)
     * @param eventId The event ID to register.
     * @param handler The typed handler to call when the event is received.
     * @param autoRemove True to automatically remove the handler after it has been called.
     */
    template <typename EventType = SIMCONNECT_RECV_EVENT>
    void registerEventHandler(DWORD eventId, 
                             std::function<void(const EventType&)> handler,
                             bool autoRemove = false) {
        this->registerHandler(eventId, [handler](const SIMCONNECT_RECV& msg) {
            handler(reinterpret_cast<const EventType&>(msg));
        }, autoRemove);
    }


    /**
     * Remove a handler for a specific event ID.
     * 
     * @param eventId The event ID to remove the handler for.
     */
    void removeEventHandler(DWORD eventId) {
        this->removeHandler(eventId);
    }

#pragma endregion

#pragma region Event Sending

    /**
     * Send an Event.
     * 
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data The optional data to send with the event.
     */
    void sendEvent(event evt, SIMCONNECT_NOTIFICATION_GROUP_ID groupId, unsigned long data = 0) {
        this->connection().transmitClientEvent(SIMCONNECT_OBJECT_ID_USER, evt, groupId, data);
    }


    /**
     * Send an Event, not in a group, but with a priority.
     * 
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data The optional data to send with the event.
     */
    void sendEventWithPriority(event evt, unsigned long priority, unsigned long data = 0) {
        this->connection().transmitClientEventWithPriority(SIMCONNECT_OBJECT_ID_USER, evt, priority, data);
    }


    /**
     * Send an Event to a specific object.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data The optional data to send with the event.
     */
    void sendEventToObject(SIMCONNECT_OBJECT_ID objectId, event evt, SIMCONNECT_NOTIFICATION_GROUP_ID groupId, unsigned long data = 0) {
        this->connection().transmitClientEvent(objectId, evt, groupId, data);
    }


    /**
     * Send an Event to a specific object, not in a group, but with a priority.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param priority The priority to send the event with.
     * @param data The optional data to send with the event.
     */
    void sendEventToObjectWithPriority(SIMCONNECT_OBJECT_ID objectId, event evt, unsigned long priority, unsigned long data = 0) {
        this->connection().transmitClientEventWithPriority(objectId, evt, priority, data);
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
     */
    void sendEvent(event evt, SIMCONNECT_NOTIFICATION_GROUP_ID groupId, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEvent(SIMCONNECT_OBJECT_ID_USER, evt, groupId, data0, data1, data2, data3, data4);
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
     */
    void sendEventWithPriority(event evt, unsigned long priority, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEventWithPriority(SIMCONNECT_OBJECT_ID_USER, evt, priority, data0, data1, data2, data3, data4);
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
     */
    void sendEventToObject(SIMCONNECT_OBJECT_ID objectId, event evt, SIMCONNECT_NOTIFICATION_GROUP_ID groupId, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEvent(objectId, evt, groupId, data0, data1, data2, data3, data4);
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
     */
    void sendEventToObjectWithPriority(SIMCONNECT_OBJECT_ID objectId, event evt, unsigned long priority, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        this->connection().transmitClientEventWithPriority(objectId, evt, priority, data0, data1, data2, data3, data4);
    }

#pragma endregion

};

} // namespace SimConnect