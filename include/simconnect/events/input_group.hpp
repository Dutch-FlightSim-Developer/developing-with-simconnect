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
#include <simconnect/events/event_group.hpp>
#include <simconnect/util/statefull_object.hpp>

#include <atomic>
#include <string_view>
#include <optional>

namespace SimConnect {


/**
 * An input group is a group of Input Events that can be enabled or disabled together.
 * 
 * **NOTE** Because Input Groups manage their own state, they should not be copied.
 * 
 * @tparam M The type of the SimConnect message handler.
 */
template <class M, bool EnableEventGroupHandler>
class InputGroup : public StateFullObject, public EventGroup {
public:
    using handler_type = EventHandler<M, EnableEventGroupHandler>;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using mutex_type = typename M::mutex_type;
    using guard_type = typename M::guard_type;


private:
    EventHandler<M, EnableEventGroupHandler>& handler_;
    InputGroupId id_;
    std::optional<Events::Priority> priority_;

    mutex_type mutex_;
    bool created_{ false };
    bool enabled_{ false };


    /**
     * Create the input group in SimConnect by setting its priority and state.
     * This method is internal and assumes the mutex is already locked.
     * 
     * **NOTE** SimConnect will consider the group unknown until events have been added to it.
     * 
     * @returns True if the input group was created successfully or already exists.
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
    /**
     * Construct a new Input Group instance.
     * 
     * @param handler The event handler to associate with this input group.
     */
    InputGroup(EventHandler<M, EnableEventGroupHandler>& handler) : handler_(handler), id_(nextId()), priority_(std::nullopt)
    {
    }

    /**
     * No copies allowed.
     */
    InputGroup(const InputGroup&) = delete;
    InputGroup& operator=(const InputGroup&) = delete;

    /**
     * Custom move constructor to solve the unmovability of the mutex.
     */
    InputGroup(InputGroup&& other) noexcept
        : handler_(other.handler_),
          id_(other.id_),
          priority_(std::move(other.priority_)),
          created_(other.created_),
          enabled_(other.enabled_)
    {
        // mutex_ is default-constructed; cannot be moved
    }

    /**
     * Custom move assignment operator to solve the unmovability of the mutex.
     */
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

#pragma region Accessors

    /**
     * Get the ID of this input group.
     * 
     * @returns The input group ID.
     */
    [[nodiscard]]
    InputGroupId id() const noexcept { return id_; }


    /**
     * Implicit conversion to InputGroupId.
     * 
     * @returns The input group ID.
     */
    [[nodiscard]]
    operator InputGroupId() const noexcept { return id_; }


    /**
     * Get the priority of this input group.
     * 
     * @returns The priority of this input group.
     */
    [[nodiscard]]
    Events::Priority priority() const noexcept { 
        return priority_.value_or(Events::defaultPriority); 
    }


    /**
     * Check if this input group has a priority set.
     * 
     * @returns True if this input group has a priority set.
     */
    [[nodiscard]]
    bool hasPriority() const noexcept {
        return priority_.has_value();
    }


    /**
     * Check if this input group has been created in SimConnect.
     * 
     * @returns True if this input group has been created.
     */
    [[nodiscard]]
    bool isCreated() const noexcept {
        return created_;
    }


    /**
     * Check if this input group is enabled.
     * 
     * @returns True if this input group is enabled.
     */
    [[nodiscard]]
    bool isEnabled() const noexcept {
        return enabled_;
    }

#pragma endregion

#pragma region Priority Setters


    /**
     * Set the priority of this input group.
     * 
     * @param priority The priority to set.
     * @returns A reference to this input group.
     */
    InputGroup& withPriority(Events::Priority priority) & {
        priority_ = priority;
        return *this;
    }
    InputGroup&& withPriority(Events::Priority priority) && {
        priority_ = priority;
        return std::move(*this);
    }


    /**
     * Set the priority of this input group to highest.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& withHighestPriority() & {
        return withPriority(Events::highestPriority);
    }
    InputGroup&& withHighestPriority() && {
        return std::move(*this).withPriority(Events::highestPriority);
    }


    /**
     * Set the priority of this input group to highest maskable.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& withMaskablePriority() & {
        return withPriority(Events::highestMaskablePriority);
    }
    InputGroup&& withMaskablePriority() && {
        return std::move(*this).withPriority(Events::highestMaskablePriority);
    }


    /**
     * Set the priority of this input group to standard.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& withStandardPriority() & {
        return withPriority(Events::standardPriority);
    }
    InputGroup&& withStandardPriority() && {
        return std::move(*this).withPriority(Events::standardPriority);
    }


    /**
     * Set the priority of this input group to default.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& withDefaultPriority() & {
        return withPriority(Events::defaultPriority);
    }
    InputGroup&& withDefaultPriority() && {
        return std::move(*this).withPriority(Events::defaultPriority);
    }


    /**
     * Set the priority of this input group to lowest.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& withLowestPriority() & {
        return withPriority(Events::lowestPriority);
    }
    InputGroup&& withLowestPriority() && {
        return std::move(*this).withPriority(Events::lowestPriority);
    }

#pragma endregion

#pragma region Enable/Disable

    /**
     * Enable this input group.
     * 
     * @returns A reference to this input group.
     */
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


    /**
     * Disable this input group.
     * 
     * @returns A reference to this input group.
     */
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

#pragma endregion

#pragma region Adding Events

    /**
     * Add an input event to this input group.
     * 
     * Maps an input event (keyboard key, joystick button, etc.) to a client event.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @returns A reference to this input group.
     */
    InputGroup& addEvent(event evt, std::string_view inputEvent) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_));
        if (succeeded()) {
            state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        }
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event evt, std::string_view inputEvent) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_));
        if (succeeded()) {
            state(handler_.connection().addClientEventToNotificationGroup(id_, evt));
        }
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add an input event with a down value to this input group.
     * 
     * Maps an input event (keyboard key, joystick button, etc.) to a client event with a value.
     * The value is sent when the input goes DOWN.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param downValue The value to send when the input goes DOWN.
     * @returns A reference to this input group.
     */
    InputGroup& addEvent(event evt, std::string_view inputEvent, unsigned long downValue) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event evt, std::string_view inputEvent, unsigned long downValue) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add an input event with separate up event to this input group.
     * 
     * Maps an input event to two client events: one for DOWN and one for UP.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param downEvt The client event to send when input goes DOWN.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param upEvt The client event to send when input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addEvent(event downEvt, std::string_view inputEvent, event upEvt) & {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEvent(downEvt, upEvt, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event downEvt, std::string_view inputEvent, event upEvt) && {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEvent(downEvt, upEvt, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add an input event with down and up values to this input group.
     * 
     * Maps an input event to a client event with different values for DOWN and UP.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param downValue The value to send when the input goes DOWN.
     * @param upValue The value to send when the input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addEvent(event evt, std::string_view inputEvent, unsigned long downValue, unsigned long upValue) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, evt, upValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event evt, std::string_view inputEvent, unsigned long downValue, unsigned long upValue) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, evt, upValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add an input event with separate up event and values to this input group.
     * 
     * Maps an input event to two different client events with different values for DOWN and UP.
     * This provides full control over the mapping.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param downEvt The client event to send when input goes DOWN.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param upEvt The client event to send when input goes UP.
     * @param downValue The value to send when the input goes DOWN.
     * @param upValue The value to send when the input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addEvent(event downEvt, std::string_view inputEvent, event upEvt, 
                        unsigned long downValue, unsigned long upValue) & {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEventWithValue(downEvt, downValue, upEvt, upValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addEvent(event downEvt, std::string_view inputEvent, event upEvt, 
                         unsigned long downValue, unsigned long upValue) && {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEventWithValue(downEvt, downValue, upEvt, upValue, inputEvent, id_));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }

#pragma endregion

#pragma region Adding Maskable Events

    /**
     * Add a maskable input event to this input group.
     * 
     * Maps an input event (keyboard key, joystick button, etc.) to a client event.
     * Maskable events can be overridden by higher priority input groups.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @returns A reference to this input group.
     */
    InputGroup& addMaskableEvent(event evt, std::string_view inputEvent) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addMaskableEvent(event evt, std::string_view inputEvent) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEvent(evt, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add a maskable input event with a down value to this input group.
     * 
     * Maps an input event to a client event with a value.
     * The value is sent when the input goes DOWN.
     * Maskable events can be overridden by higher priority input groups.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param downValue The value to send when the input goes DOWN.
     * @returns A reference to this input group.
     */
    InputGroup& addMaskableEvent(event evt, std::string_view inputEvent, unsigned long downValue) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addMaskableEvent(event evt, std::string_view inputEvent, unsigned long downValue) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add a maskable input event with separate up event to this input group.
     * 
     * Maps an input event to two client events: one for DOWN and one for UP.
     * Maskable events can be overridden by higher priority input groups.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param downEvt The client event to send when input goes DOWN.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param upEvt The client event to send when input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addMaskableEvent(event downEvt, std::string_view inputEvent, event upEvt) & {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEvent(downEvt, upEvt, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addMaskableEvent(event downEvt, std::string_view inputEvent, event upEvt) && {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEvent(downEvt, upEvt, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add a maskable input event with down and up values to this input group.
     * 
     * Maps an input event to a client event with different values for DOWN and UP.
     * Maskable events can be overridden by higher priority input groups.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param evt The client event to map to.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param downValue The value to send when the input goes DOWN.
     * @param upValue The value to send when the input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addMaskableEvent(event evt, std::string_view inputEvent, unsigned long downValue, unsigned long upValue) & {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, evt, upValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addMaskableEvent(event evt, std::string_view inputEvent, unsigned long downValue, unsigned long upValue) && {
        guard_type lock(mutex_);

        // Automatically map the event if not already mapped
        handler_.mapEvent(evt);
        state(handler_.connection().mapInputEventToClientEventWithValue(evt, downValue, evt, upValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }


    /**
     * Add a maskable input event with separate up event and values to this input group.
     * 
     * Maps an input event to two different client events with different values for DOWN and UP.
     * Maskable events can be overridden by higher priority input groups.
     * This provides full control over the mapping.
     * 
     * Note: if the priority of this input group is not set yet, it will be set to default.
     * 
     * @param downEvt The client event to send when input goes DOWN.
     * @param inputEvent The input event name (e.g., "VK_SPACE", "joystick:0:button:0").
     * @param upEvt The client event to send when input goes UP.
     * @param downValue The value to send when the input goes DOWN.
     * @param upValue The value to send when the input goes UP.
     * @returns A reference to this input group.
     */
    InputGroup& addMaskableEvent(event downEvt, std::string_view inputEvent, event upEvt, 
                                unsigned long downValue, unsigned long upValue) & {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEventWithValue(downEvt, downValue, upEvt, upValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return *this;
    }
    InputGroup&& addMaskableEvent(event downEvt, std::string_view inputEvent, event upEvt, 
                                 unsigned long downValue, unsigned long upValue) && {
        guard_type lock(mutex_);

        // Automatically map both events if not already mapped
        handler_.mapEvent(downEvt);
        handler_.mapEvent(upEvt);
        state(handler_.connection().mapInputEventToClientEventWithValue(downEvt, downValue, upEvt, upValue, inputEvent, id_, true));
        if (succeeded()) {
            createInternal();
        }
        
        return std::move(*this);
    }

#pragma endregion

#pragma region Removing Events

    /**
     * Remove an input event mapping from this input group.
     * 
     * Removes the mapping for a specific input event (keyboard key, joystick button, etc.).
     * 
     * @param inputEvent The input event name to remove (e.g., "VK_SPACE", "joystick:0:button:0").
     * @returns A reference to this input group.
     */
    InputGroup& removeEvent(std::string_view inputEvent) & {
        state(handler_.connection().removeInputEvent(id_, inputEvent));

        return *this;
    }
    InputGroup&& removeEvent(std::string_view inputEvent) && {
        state(handler_.connection().removeInputEvent(id_, inputEvent));

        return std::move(*this);
    }


    /**
     * Clear all input event mappings from this input group.
     * 
     * @returns A reference to this input group.
     */
    InputGroup& clear() & {
        state(handler_.connection().clearInputGroup(id_));

        return *this;
    }
    InputGroup&& clear() && {
        state(handler_.connection().clearInputGroup(id_));

        return std::move(*this);
    }

#pragma endregion

};


// Implementation of EventHandler::createInputGroup()
// This must be defined after InputGroup is fully defined to break circular dependency
template <class M, bool EnableEventGroupHandler>
InputGroup<M, EnableEventGroupHandler> EventHandler<M, EnableEventGroupHandler>::createInputGroup() {
    return InputGroup<M, EnableEventGroupHandler>(*this);
}

} // namespace SimConnect