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

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <optional>


namespace SimConnect {

/**
 * A notification group is a group of events that can be enabled or disabled together.
 */
template <class M>
class NotificationGroup {
    EventHandler<M>& handler_;
    SIMCONNECT_NOTIFICATION_GROUP_ID id_;
    std::optional<unsigned long> priority_;


    inline static std::atomic<SIMCONNECT_NOTIFICATION_GROUP_ID> nextId_{ 0 };


public:
    NotificationGroup(EventHandler<M>& handler) : handler_(handler), id_(++nextId_), priority_(std::nullopt)
    {
    }
    NotificationGroup(const NotificationGroup&) = default;
    NotificationGroup(NotificationGroup&&) = default;
    NotificationGroup& operator=(const NotificationGroup&) = default;
    NotificationGroup& operator=(NotificationGroup&&) = default;
    ~NotificationGroup() = default;


    [[nodiscard]]
    SIMCONNECT_NOTIFICATION_GROUP_ID id() const noexcept { return id_; }

    [[nodiscard]]
    operator SIMCONNECT_NOTIFICATION_GROUP_ID() const noexcept { return id_; }

    [[nodiscard]]
    unsigned long priority() const noexcept { 
        return priority_.value_or(SIMCONNECT_GROUP_PRIORITY_DEFAULT); 
    }

    [[nodiscard]]
    bool hasPriority() const noexcept { 
        return priority_.has_value(); 
    }


    NotificationGroup& withPriority(unsigned long priority) {
        priority_ = priority;
        handler_.connection().setNotificationGroupPriority(id_, static_cast<int>(priority));
        return *this;
    }
    NotificationGroup& withHighestPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_HIGHEST);
    }
    NotificationGroup& withMaskablePriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE);
    }
    NotificationGroup& withStandardPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_STANDARD);
    }
    NotificationGroup& withDefaultPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_DEFAULT);
    }
    NotificationGroup& withLowestPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_LOWEST);
    }


    NotificationGroup& addEvent(event evt) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evt);
        return *this;
    }
    NotificationGroup& addEvent(int evtId) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(static_cast<unsigned long>(evtId));
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evtId);
        return *this;
    }
    NotificationGroup& addEvent(std::string evtName) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evt);
        return *this;
    }


    NotificationGroup& addMaskableEvent(event evt) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evt, true);
        return *this;
    }
    NotificationGroup& addMaskableEvent(int evtId) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(static_cast<unsigned long>(evtId));
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evtId, true);
        return *this;
    }
    NotificationGroup& addMaskableEvent(std::string evtName) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        auto evt = event::get(evtName);
        handler_.mapEvent(evt);
        handler_.connection().addClientEventToNotificationGroup(id_, evt, true);
        return *this;
    }


    NotificationGroup& removeEvent(event evt) {
        handler_.connection().removeClientEventFromNotificationGroup(id_, evt);
        return *this;
    }
    NotificationGroup& removeEvent(int evtId) {
        handler_.connection().removeClientEventFromNotificationGroup(id_, evtId);
        return *this;
    }
    NotificationGroup& removeEvent(std::string evtName) {
        handler_.connection().removeClientEventFromNotificationGroup(id_, event::get(evtName));
        return *this;
    }


    void clear() {
        handler_.connection().clearNotificationGroup(id_);
    }


    void request() {
        handler_.connection().requestNotificationGroup(id_);
    }


    NotificationGroup& sendEvent(event evt, unsigned long data = 0) {
        handler_.sendEvent(evt, id_, data);
        return *this;
    }

    NotificationGroup& sendEventToObject(SIMCONNECT_OBJECT_ID objectId, event evt, unsigned long data = 0) {
        handler_.sendEventToObject(objectId, evt, id_, data);
        return *this;
    }

    NotificationGroup& sendEvent(event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        handler_.sendEvent(evt, id_, data0, data1, data2, data3, data4);
        return *this;
    }

    NotificationGroup& sendEventToObject(SIMCONNECT_OBJECT_ID objectId, event evt, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        handler_.sendEventToObject(objectId, evt, id_, data0, data1, data2, data3, data4);
        return *this;
    }
    
};


// Implementation of EventHandler::createNotificationGroup()
// This must be defined after NotificationGroup is fully defined to break circular dependency
template <class M>
NotificationGroup<M> EventHandler<M>::createNotificationGroup() {
    return NotificationGroup<M>(*this);
}

} // namespace SimConnect