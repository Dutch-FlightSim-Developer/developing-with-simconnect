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

#include <simconnect.hpp>


#include <atomic>
#include <map>
#include <string>


namespace SimConnect {


/**
 * An event is a named event that can be sent to the simulator.
 */
class event {
private:
    int id_;

    static std::atomic_int nextId_;                     ///< The next ID to assign to an event.

    static std::map<std::string, int> eventsByName_;    ///< A static map of the events by name.
    static std::map<int, std::string> eventsById_;      ///< A static map of the events by ID.


    /**
     * Construct an event with the given ID. This is the only constructor that is allowed, and it is private to ensure that
     * the only way to get an event is through the static get() methods.
     */
    event(int id) : id_(id) {}

    /**
     * The default constructor is deleted because an event MUST always have an Id.
     */
    event() = delete;

    // No move semantics, because an event MUST always have an Id.
    event(event&&) = delete;
    event& operator=(event&&) = delete;

public:


    /**
     * Destructor can be default.
     */
    constexpr ~event() = default;


    // Copying is allowed, moving is not because an event MUST always have an Id.

    constexpr event(const event&) = default;
    constexpr event& operator=(const event&) = default;


    /**
     * Get an event by name. If the event does not exist yet, it will be created.
     * 
     * @param name The name of the event.
     * @returns The event.
     */
    static event get(std::string name) {
        auto it = eventsByName_.find(name);
        if (it != eventsByName_.end()) {
            return event(it->second);
        }
        auto id = ++nextId_;
        eventsByName_[name] = id;
        eventsById_[id] = name;

        return event(id);
    }


    /**
     * Get an event by ID. If the event does not exist, an exception will be thrown.
     * 
     * @param id The ID of the event.
     * @returns The event.
     * @throws UnknownEvent if the event does not exist.
     */
    static event get(int id) {
        auto it = eventsById_.find(id);

        if (it != eventsById_.end()) {
            return event(id);       // We don't mind copies.
        }
        throw UnknownEvent(id);
    }


    /**
     * Get the ID of the event.
     * 
     * @returns The ID of the event.
     */
    [[nodiscard]]
    constexpr int id() const noexcept { return id_; }


    /**
     * Convert an event to an integer. This is useful for passing the event to SimConnect functions.
     * 
     * @returns The ID of the event as an integer.
     */
    [[nodiscard]]
    constexpr operator int() const noexcept { return id_; }


    /**
     * Get the name of the event.
     * 
     * @returns The name of the event.
     * @throws UnknownEvent if the event does not exist.
     */
    [[nodiscard]]
    const std::string& name() const {
        auto it = eventsById_.find(id_);

        if (it != eventsById_.end()) {
            return it->second;
        }
        throw UnknownEvent(id_);
    }


    /**
     * Compare two events for equality.
     * 
     * @param other The other event.
     * @returns True if the events are equal.
     */
    [[nodiscard]]
    constexpr bool operator==(const event& other) const noexcept { return id_ == other.id_; }
    
    /**
     * Compare two events for ordering.
     * 
     * @param other The other event.
     * @returns The comparison result.
     */
    constexpr auto operator<=>(const event& other) const noexcept { return id_ <=> other.id_; }
};

}