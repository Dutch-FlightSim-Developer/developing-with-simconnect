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

#include <atomic>
#include <string_view>
#include <optional>

namespace SimConnect {


/**
 * An input group is a group of Input Events that can be enabled or disabled together.
 */
template <class M>
class InputGroup : public StateFullObject {
public:
    using handler_type = EventHandler<M>;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using mutex_type = typename M::mutex_type;
    using guard_type = typename M::guard_type;


private:
    EventHandler<M>& handler_;
    InputGroupId id_;
    std::optional<Events::Priority> priority_;

    mutex_type mutex_;
    bool created_{ false };
    bool enabled_{ false };

    inline static std::atomic<InputGroupId> nextId_{ 0 };


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
        state(handler_.connection().setInputGroupPriority(id_, priority_.value()));
        if (succeeded()) {
            state(handler_.connection().setInputGroupState(id_, enabled_));
        }
        if (succeeded()) {
            created_ = true;
        }

        return succeeded();
    }


public:
    InputGroup(EventHandler<M>& handler) : handler_(handler), id_(++nextId_), priority_(std::nullopt)
    {
    }
    InputGroup(const InputGroup&) = delete;
    InputGroup(InputGroup&& other) noexcept
        : handler_(other.handler_),
          id_(other.id_),
          priority_(std::move(other.priority_)),
          created_(other.created_),
          enabled_(other.enabled_)
    {
        // mutex_ is default-constructed; cannot be moved
    }
    InputGroup& operator=(const InputGroup&) = delete;
    InputGroup& operator=(InputGroup&& other) noexcept {
        if (this != &other) {
            guard_type lock1(mutex_, std::defer_lock);
            guard_type lock2(other.mutex_, std::defer_lock);
            std::lock(lock1, lock2);
            
            handler_ = other.handler_;
            id_ = other.id_;
            priority_ = std::move(other.priority_);
            created_ = other.created_;
            enabled_ = other.enabled_;
        }
        return *this;
    }
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


    InputGroup& withPriority(Events::Priority priority) & {
        priority_ = priority;
        return *this;
    }
    InputGroup&& withPriority(Events::Priority priority) && {
        priority_ = priority;
        return std::move(*this);
    }
    InputGroup& withHighestPriority() & {
        return withPriority(Events::highestPriority);
    }
    InputGroup&& withHighestPriority() && {
        return std::move(*this).withPriority(Events::highestPriority);
    }
    InputGroup& withMaskablePriority() & {
        return withPriority(Events::highestMaskablePriority);
    }
    InputGroup&& withMaskablePriority() && {
        return std::move(*this).withPriority(Events::highestMaskablePriority);
    }
    InputGroup& withStandardPriority() & {
        return withPriority(Events::standardPriority);
    }
    InputGroup&& withStandardPriority() && {
        return std::move(*this).withPriority(Events::standardPriority);
    }
    InputGroup& withDefaultPriority() & {
        return withPriority(Events::defaultPriority);
    }
    InputGroup&& withDefaultPriority() && {
        return std::move(*this).withPriority(Events::defaultPriority);
    }
    InputGroup& withLowestPriority() & {
        return withPriority(Events::lowestPriority);
    }
    InputGroup&& withLowestPriority() && {
        return std::move(*this).withPriority(Events::lowestPriority);
    }

    InputGroup& enable() & {
        guard_type lock(mutex_);

        enabled_ = true;
        if (created_) {
            state(handler_.connection().setInputGroupState(id_, enabled_));
        }
        return *this;
    }
    InputGroup&& enable() && {
        guard_type lock(mutex_);

        enabled_ = true;
        if (created_) {
            state(handler_.connection().setInputGroupState(id_, enabled_));
        }
        return std::move(*this);
    }
    InputGroup& disable() & {
        guard_type lock(mutex_);

        enabled_ = false;
        if (created_) {
            state(handler_.connection().setInputGroupState(id_, enabled_));
        }
        return *this;
    }
    InputGroup&& disable() && {
        guard_type lock(mutex_);

        enabled_ = false;
        if (created_) {
            state(handler_.connection().setInputGroupState(id_, enabled_));
        }
        return std::move(*this);
    }


    InputGroup& addEvent(event evt, std::string_view inputEvent) & {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        if (succeeded()) {
            handler_.mapEvent(evt);
        }
        if (succeeded()) {
            state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_));
        }
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event evt, std::string_view inputEvent) && {
        if (!priority_.has_value()) {
            withDefaultPriority();
        }
        // Automatically map the event if not already mapped
        if (succeeded()) {
            handler_.mapEvent(evt);
        }
        if (succeeded()) {
            state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_));
        }
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }
};


// Implementation of EventHandler::createInputGroup()
// This must be defined after InputGroup is fully defined to break circular dependency
template <class M>
InputGroup<M> EventHandler<M>::createInputGroup() {
    return InputGroup<M>(*this);
}

} // namespace SimConnect