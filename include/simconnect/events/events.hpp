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


class event {
private:
    int id_;

    static std::atomic_int nextId_;

    static std::map<std::string, int> eventsByName_;
    static std::map<int, std::string> eventsById_;

    event(int id) : id_(id) {}

public:
    event(const event&) = default;
    event(event&&) = delete;
    event& operator=(const event&) = default;
    event& operator=(event&&) = delete;

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


    static event get(int id) {
        auto it = eventsById_.find(id);
        if (it != eventsById_.end()) {
            return event(id);
        }
        throw UnknownEvent(id);
    }

    int id() const noexcept { return id_; }
    const std::string& name() const {
        auto it = eventsById_.find(id_);
        if (it != eventsById_.end()) {
            return it->second;
        }
        throw UnknownEvent(id_);
    }

    bool operator==(const event& other) const noexcept { return id_ == other.id_; }
    auto operator<=>(const event& other) const noexcept { return id_ <=> other.id_; }
};

}