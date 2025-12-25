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
 * An input group is a group of Input Events that can be enabled or disabled together.
 */
template <class M>
class InputGroup : public StateFullObject {
    EventHandler<M>& handler_;
    InputGroupId id_;
    std::optional<Events::Priority> priority_;

    inline static std::atomic<InputGroupId> nextId_{ 0 };


public:
    InputGroup(EventHandler<M>& handler) : handler_(handler), id_(++nextId_), priority_(std::nullopt)
    {
    }
    InputGroup(const InputGroup&) = default;
    InputGroup(InputGroup&&) = default;
    InputGroup& operator=(const InputGroup&) = default;
    InputGroup& operator=(InputGroup&&) = default;
    ~InputGroup() = default;


    [[nodiscard]]
    InputGroupId id() const noexcept { return id_; }
    [[nodiscard]]
    operator InputGroupId() const noexcept { return id_; }

    [[nodiscard]]
    Events::Priority priority() const noexcept { 
        return priority_.value_or(Events::defaultPriority); 
    }
    [[nodiscard]]
    bool hasPriority() const noexcept {
        return priority_.has_value();
    }


    InputGroup& withPriority(Events::Priority priority) {
        priority_ = priority;
        state(handler_.connection().setInputGroupPriority(id_, priority));
        return *this;
    }
    InputGroup& withHighestPriority() {
        return withPriority(Events::highestPriority);
    }
    InputGroup& withMaskablePriority() {
        return withPriority(Events::highestMaskablePriority);
    }
    InputGroup& withStandardPriority() {
        return withPriority(Events::standardPriority);
    }
    InputGroup& withDefaultPriority() {
        return withPriority(Events::defaultPriority);
    }
    InputGroup& withLowestPriority() {
        return withPriority(Events::lowestPriority);
    }

    InputGroup& enable() {
        state(handler_.connection().setInputGroupState(id_, Events::on));
        return *this;
    }
    InputGroup& disable() {
        state(handler_.connection().setInputGroupState(id_, Events::off));
        return *this;
    }


    InputGroup& addEvent(event evt, std::string inputEvent) {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        if (succeeded()) {
            handler_.mapEvent(evt); // TODO: state?
        }
        if (succeeded()) {
            state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_));
        }
        if (succeeded()) {
            state(handler_.connection().addClientEventToInputGroup(id_, evt, inputEvent));
        }
        
        return *this;
    }
};


// Implementation of EventHandler::createNotificationGroup()
// This must be defined after NotificationGroup is fully defined to break circular dependency
template <class M>
InputGroup<M> EventHandler<M>::createInputGroup() {
    return InputGroup<M>(*this);
}

} // namespace SimConnect