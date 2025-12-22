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
#include <mutex>
#include <set>
#include <string>


namespace SimConnect {


using EventId = unsigned long;


/**
 * An event is a named event that can be sent to the simulator.
 */
class event {
private:
    EventId id_;

    inline static std::atomic<EventId> nextId_{ 0 };                  ///< The next ID to assign to an event.

    inline static std::map<std::string, EventId> eventsByName_{};    ///< A static map of the events by name.
    inline static std::map<EventId, std::string> eventsById_{};      ///< A static map of the events by ID.
    inline static std::set<EventId> mappedEvents_{};                 ///< Set of event IDs that have been mapped to SimConnect.
    
    inline static std::mutex registryMutex_;                                ///< Mutex for thread-safe access to static collections.


    /**
     * Construct an event with the given ID. This is the only constructor that is allowed, and it is private to ensure that
     * the only way to get an event is through the static get() methods.
     */
    event(EventId id) : id_(id) {}

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
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        auto it = eventsByName_.find(name);
        if (it != eventsByName_.end()) {
            return event(it->second);
        }
        auto id = ++nextId_;
        eventsByName_[name] = id;
        eventsById_[id] = name;
        // No need to add to mappedEvents_ - absence means not mapped

        return event(id);
    }


    /**
     * Get an event by ID. If the event does not exist, an exception will be thrown.
     * 
     * @param id The ID of the event.
     * @returns The event.
     * @throws UnknownEvent if the event does not exist.
     */
    static event get(EventId id) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
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
     * @throws UnknownEvent if the event does not exist.
     */
    [[nodiscard]]
    const std::string& name() const {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        auto it = eventsById_.find(id_);

        if (it != eventsById_.end()) {
            return it->second;
        }
        throw UnknownEvent(id_);
    }


    /**
     * Check if this event has been mapped to SimConnect.
     * 
     * @returns True if the event has been mapped.
     */
    [[nodiscard]]
    bool isMapped() const {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        return mappedEvents_.contains(id_);
    }


    /**
     * Mark this event as mapped to SimConnect.
     * This should be called after successfully calling SimConnect_MapClientEventToSimEvent.
     */
    void setMapped() {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        mappedEvents_.insert(id_);
    }


    /**
     * Reset the mapped status of this event.
     * This might be needed when reconnecting to SimConnect.
     */
    void clearMapped() {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        mappedEvents_.erase(id_);
    }


    /**
     * Clear all mapped status flags.
     * This should be called when disconnecting from SimConnect to allow re-mapping on reconnect.
     */
    static void clearAllMappedFlags() {
        std::lock_guard<std::mutex> lock(registryMutex_);
        
        mappedEvents_.clear();
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