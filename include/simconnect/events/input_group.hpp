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

#include <simconnect/simconnect_error.hpp>
#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <optional>

namespace SimConnect {


using InputGroupId = unsigned long;


/**
 * An input group is a group of Input Events that can be enabled or disabled together.
 */
template <class M>
class InputGroup {
    EventHandler<M>& handler_;
    InputGroupId id_;
    std::optional<unsigned long> priority_;

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
    unsigned long priority() const noexcept { 
        return priority_.value_or(SIMCONNECT_GROUP_PRIORITY_DEFAULT); 
    }
    [[nodiscard]]
    bool hasPriority() const noexcept {
        return priority_.has_value();
    }


    Result<InputGroup&> withPriority(unsigned long priority) {
        priority_ = priority;
        auto result = handler_.connection().setInputGroupPriority(id_, priority);
        if (result.hasError()) {
            return result.error();
        }
        return *this;
    }
    Result<InputGroup&> withHighestPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_HIGHEST);
    }
    Result<InputGroup&> withMaskablePriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE);
    }
    Result<InputGroup&> withStandardPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_STANDARD);
    }
    Result<InputGroup&> withDefaultPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_DEFAULT);
    }
    Result<InputGroup&> withLowestPriority() {
        return withPriority(SIMCONNECT_GROUP_PRIORITY_LOWEST);
    }

    Result<InputGroup&> enable() {
        auto result = handler_.connection().setInputGroupState(id_, SIMCONNECT_STATE_ON);
        if (result.hasError()) {
            return result.error();
        }
        return *this;
    }
    Result<InputGroup&> disable() {
        auto result = handler_.connection().setInputGroupState(id_, SIMCONNECT_STATE_OFF);
        if (result.hasError()) {
            return result.error();
        }
        return *this;
    }


    Result<InputGroup&> addEvent(event evt, std::string inputEvent) {
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