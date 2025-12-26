#pragma once
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

#include <simconnect/simconnect.hpp>
#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/util/statefull_object.hpp>

#include <optional>


namespace SimConnect {


/**
 * A notification group is a group of events that can be enabled or disabled together.
 * 
 * @tparam M The type of the SimConnect message handler.
 */
template <class M>
class NotificationGroup : public StateFullObject {
public:
    using handler_type = EventHandler<M>;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using mutex_type = typename M::mutex_type;
    using guard_type = typename M::guard_type;


private:
    EventHandler<M>& handler_;
    NotificationGroupId id_;
    std::optional<Events::Priority> priority_;

    mutex_type mutex_;
    bool created_{ false };

    inline static std::atomic<NotificationGroupId> nextId_{ 0 };


    /**
     * Create the notification group in SimConnect. This method is internal and assumes the mutex is already locked.
     */
    bool createInternal() {
        if (created_) {
            return true;
        }
        if (!priority_.has_value()) {
            priority_ = Events::defaultPriority;
        }
        state(handler_.connection().setNotificationGroupPriority(id_, priority_.value()));
        if (succeeded()) {
            created_ = true;
        }

        return succeeded();
    }


public:
    NotificationGroup(EventHandler<M>& handler) : handler_(handler), id_(++nextId_), priority_(std::nullopt)
    {
    }
    NotificationGroup(const NotificationGroup&) = delete;
    NotificationGroup(NotificationGroup&& other) noexcept
        : handler_(other.handler_),
          id_(other.id_),
          priority_(std::move(other.priority_)),
          created_(other.created_)
    {
        // mutex_ is default-constructed; cannot be moved
    }
    NotificationGroup& operator=(const NotificationGroup&) = delete;
    NotificationGroup& operator=(NotificationGroup&& other) noexcept {
        if (this != &other) {
            guard_type lock1(mutex_, std::defer_lock);
            guard_type lock2(other.mutex_, std::defer_lock);
            std::lock(lock1, lock2);
            
            handler_ = other.handler_;
            id_ = other.id_;
            priority_ = std::move(other.priority_);
            created_ = other.created_;
        }
        return *this;
    }
    ~NotificationGroup() = default;


    /**
     * Get the ID of this notification group.
     * 
     * @returns The notification group ID.
     */
    [[nodiscard]]
    NotificationGroupId id() const noexcept { return id_; }


    /**
     * Implicit conversion to NotificationGroupId.
     * 
     * @returns The notification group ID.
     */
    [[nodiscard]]
    operator NotificationGroupId() const noexcept { return id_; }


    /**
     * Get the priority of this notification group.
     * 
     * @returns The priority of this notification group.
     */
    [[nodiscard]]
    Events::Priority priority() const noexcept { 
        return priority_.value_or(Events::defaultPriority); 
    }


    /**
     * Check if this notification group has a priority set.
     * 
     * @returns True if this notification group has a priority set.
     */
    [[nodiscard]]
    bool hasPriority() const noexcept { 
        return priority_.has_value(); 
    }


    /**
     * Check if this notification group has been created in SimConnect.
     * 
     * @returns True if this notification group has been created.
     */
    [[nodiscard]]
    bool isCreated() const noexcept {
        return created_;
    }


    /**
     * Set the priority of this notification group.
     * 
     * @param priority The priority to set.
     * @returns A reference to this notification group.
     */
    NotificationGroup& withPriority(Events::Priority priority) & {
        priority_ = priority;
        return *this;
    }
    NotificationGroup&& withPriority(Events::Priority priority) && {
        priority_ = priority;
        return std::move(*this);
    }
    /**
     * Set the priority of this notification group to highest.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& withHighestPriority() & {
        return withPriority(Events::highestPriority);
    }
    NotificationGroup&& withHighestPriority() && {
        return std::move(*this).withPriority(Events::highestPriority);
    }
    /**
     * Set the priority of this notification group to highest maskable.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& withMaskablePriority() & {
        return withPriority(Events::highestMaskablePriority);
    }
    NotificationGroup&& withMaskablePriority() && {
        return std::move(*this).withPriority(Events::highestMaskablePriority);
    }
    /**
     * Set the priority of this notification group to standard.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& withStandardPriority() & {
        return withPriority(Events::standardPriority);
    }
    NotificationGroup&& withStandardPriority() && {
        return std::move(*this).withPriority(Events::standardPriority);
    }
    /**
     * Set the priority of this notification group to default.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& withDefaultPriority() & {
        return withPriority(Events::defaultPriority);
    }
    NotificationGroup&& withDefaultPriority() && {
        return std::move(*this).withPriority(Events::defaultPriority);
    }
    /**
     * Set the priority of this notification group to lowest.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& withLowestPriority() & {
        return withPriority(Events::lowestPriority);
    }
    NotificationGroup&& withLowestPriority() && {
        return std::move(*this).withPriority(Events::lowestPriority);
    }


    /**
     * Add an event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evt The event to add.
     * @returns A reference to this notification group.
     */
    NotificationGroup& addEvent(event evt) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addEvent(event evt) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Add an event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evtId The ID of the event to add.
     * @returns A reference to this notification group.
     * @throws UnknownEvent if the event does not exist.
     */
    NotificationGroup& addEvent(EventId evtId) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtId);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evtId));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addEvent(EventId evtId) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtId);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evtId));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Add an event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evtName The name of the event to add.
     * @returns A reference to this notification group.
     * @throws UnknownEvent if the event does not exist.
     */
    NotificationGroup& addEvent(std::string evtName) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addEvent(std::string evtName) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Add a maskable event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evt The event to add.
     * @returns A reference to this notification group.
     */
    NotificationGroup& addMaskableEvent(event evt) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt, true));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addMaskableEvent(event evt) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt, true));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Add a maskable event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evtId The ID of the event to add.
     * @returns A reference to this notification group.
     */
    NotificationGroup& addMaskableEvent(EventId evtId) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtId);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evtId, true));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addMaskableEvent(EventId evtId) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtId);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evtId, true));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Add a maskable event to this notification group.
     * 
     * Note: if the priority of this notification group is not set yet, it will be set to default.
     * 
     * @param evtName The name of the event to add.
     * @returns A reference to this notification group.
     */
    NotificationGroup& addMaskableEvent(std::string evtName) & {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt, true));
        if (succeeded()) {
            createInternal();
        }
        return *this;
    }
    NotificationGroup&& addMaskableEvent(std::string evtName) && {
        guard_type lock(mutex_);

        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        state(handler_.connection().addClientEventToNotificationGroup(id_, evt, true));
        if (succeeded()) {
            createInternal();
        }
        return std::move(*this);
    }


    /**
     * Remove an event from this notification group.
     * 
     * @param evt The event to remove.
     * @returns A reference to this notification group.
     */
    NotificationGroup& removeEvent(event evt) & {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, evt));

        return *this;
    }
    NotificationGroup&& removeEvent(event evt) && {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, evt));

        return std::move(*this);
    }


    /**
     * Remove an event from this notification group.
     * 
     * @param evtName The name of the event to remove.
     * @returns A reference to this notification group.
     */
    NotificationGroup& removeEvent(EventId evtId) & {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, evtId));

        return *this;
    }
    NotificationGroup&& removeEvent(EventId evtId) && {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, evtId));

        return std::move(*this);
    }
    /**
     * Remove an event from this notification group.
     * 
     * @param evtName The name of the event to remove.
     * @returns A reference to this notification group.
     */
    NotificationGroup& removeEvent(std::string evtName) & {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, event::get(evtName)));

        return *this;
    }
    NotificationGroup&& removeEvent(std::string evtName) && {
        state(handler_.connection().removeClientEventFromNotificationGroup(id_, event::get(evtName)));

        return std::move(*this);
    }


    /**
     * Clear all events from this notification group.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& clear() & {
        state(handler_.connection().clearNotificationGroup(id_));

        return *this;
    }
    NotificationGroup&& clear() && {
        state(handler_.connection().clearNotificationGroup(id_));

        return std::move(*this);
    }


    /**
     * Request this notification group to be active.
     * 
     * @returns A reference to this notification group.
     */
    NotificationGroup& request() & {
        guard_type lock(mutex_);

        if (!created_) {
            handler_.logger().warning("Requesting notification group {} before it has been created.", id_);
            return *this;
        }
        state(handler_.connection().requestNotificationGroup(id_));
        return *this;
    }
    NotificationGroup&& request() && {
        guard_type lock(mutex_);

        if (!created_) {
            handler_.logger().warning("Requesting notification group {} before it has been created.", id_);
            return std::move(*this);
        }
        state(handler_.connection().requestNotificationGroup(id_));
        return std::move(*this);
    }


    /**
     * Send an event to this notification group.
     * 
     * @param evt The event to send.
     * @param data Optional data to send with the event.
     * @returns A reference to this notification group.
     */
    NotificationGroup& sendEvent(event evt, unsigned long data = 0) & {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEvent(evt, id_, data);

        return *this;
    }
    NotificationGroup&& sendEvent(event evt, unsigned long data = 0) && {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEvent(evt, id_, data);

        return std::move(*this);
    }


    /**
     * Send an event to a specific object in this notification group.
     * 
     * @param objectId The ID of the object to send the event to.
     * @param evt The event to send.
     * @param data Optional data to send with the event.
     * @returns A reference to this notification group.
     */
    NotificationGroup& sendEventToObject(SimObjectId objectId, event evt, unsigned long data = 0) & {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEventToObject(objectId, evt, id_, data);

        return *this;
    }
    NotificationGroup&& sendEventToObject(SimObjectId objectId, event evt, unsigned long data = 0) && {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEventToObject(objectId, evt, id_, data);

        return std::move(*this);
    }


    /**
     * Send an event with multiple data values to this notification group.
     * 
     * @param evt The event to send.
     * @param data0 The first data value to send.
     * @param data1 The second data value to send.
     * @param data2 The third data value to send.
     * @param data3 The fourth data value to send.
     * @param data4 The fifth data value to send.
     * @returns A reference to this notification group.
     */
    NotificationGroup& sendEvent(event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) & {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEvent(evt, id_, data0, data1, data2, data3, data4);

        return *this;
    }
    NotificationGroup&& sendEvent(event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) && {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEvent(evt, id_, data0, data1, data2, data3, data4);

        return std::move(*this);
    }


    /**
     * Send an event with multiple data values to a specific object in this notification group.
     * 
     * @param objectId The ID of the object to send the event to.
     * @param evt The event to send.
     * @param data0 The first data value to send.
     * @param data1 The second data value to send.
     * @param data2 The third data value to send.
     * @param data3 The fourth data value to send.
     * @param data4 The fifth data value to send.
     * @returns A reference to this notification group.
     */
    NotificationGroup& sendEventToObject(SimObjectId objectId, event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) & {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEventToObject(objectId, evt, id_, data0, data1, data2, data3, data4);

        return *this;
    }
    NotificationGroup&& sendEventToObject(SimObjectId objectId, event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) && {
        guard_type lock(mutex_);

        createInternal();

        handler_.sendEventToObject(objectId, evt, id_, data0, data1, data2, data3, data4);

        return std::move(*this);
    }
    
};


// Implementation of EventHandler::createNotificationGroup()
// This must be defined after NotificationGroup is fully defined to break circular dependency
template <class M>
NotificationGroup<M> EventHandler<M>::createNotificationGroup() {
    return NotificationGroup<M>(*this);
}

} // namespace SimConnect