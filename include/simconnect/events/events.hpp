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
#include <string>


namespace SimConnect {


using EventId = unsigned long;


/**
 * An event is a named event that can be sent to the simulator.
 */
class event {
private:
    EventId id_;
    std::string name_;

    inline static std::atomic<EventId> nextId_{ 0 };

    // event MUST always have an Id
    event() = delete;
    event(event&&) = delete;
    event& operator=(event&&) = delete;

public:

    struct Key {
    private:
        Key() = default;

        template<class D, bool TS, class L, bool TM> friend class Connection;

        static EventId allocate() noexcept { return ++event::nextId_; }
    };

    event(EventId id, std::string_view name, Key) : id_(id), name_(name) {}

    constexpr ~event() = default;

    constexpr event(const event&) = default;
    event& operator=(const event&) = default;


    /**
     * Get the ID of the event.
     *
     * @returns The ID of the event.
     */
    [[nodiscard]]
    constexpr EventId id() const noexcept { return id_; }


    /**
     * Convert an event to an EventId. This is useful for passing the event to SimConnect functions.
     *
     * @returns The ID of the event as an EventId.
     */
    [[nodiscard]]
    constexpr operator EventId() const noexcept { return id_; }


    /**
     * Get the name of the event.
     *
     * @returns The name of the event.
     */
    [[nodiscard]]
    const std::string& name() const noexcept { return name_; }


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