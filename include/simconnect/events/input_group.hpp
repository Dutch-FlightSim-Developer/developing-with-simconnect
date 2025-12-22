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
#include <simconnect/simconnect_error.hpp>
#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <optional>

namespace SimConnect {


/**
 * An input group is a group of Input Events that can be enabled or disabled together.
 */
template <class M>
class InputGroup {
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
        auto result = handler_.connection().setInputGroupPriority(id_, priority);
        if (result.hasError()) {
            return result.error();
        }
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
        auto result = handler_.connection().setInputGroupState(id_, Events::on);
        if (result.hasError()) {
            return result.error();
        }
        return *this;
    }
    InputGroup& disable() {
        auto result = handler_.connection().setInputGroupState(id_, Events::off);
        if (result.hasError()) {
            return result.error();
        }
        return *this;
    }


    InputGroup& addEvent(event evt, std::string inputEvent) {
        if (!priority_.has_value()) {
            auto priorityResult = withDefaultPriority();
            if (priorityResult.hasError()) {
                return priorityResult.error();
            }
        }
        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        
        auto mapResult = handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_);
        if (mapResult.hasError()) {
            return mapResult.error();
        }
        
        auto addResult = handler_.connection().addClientEventToInputGroup(id_, evt, inputEvent);
        if (addResult.hasError()) {
            return addResult.error();
        }
        
        return *this;
    }
};

} // namespace SimConnect