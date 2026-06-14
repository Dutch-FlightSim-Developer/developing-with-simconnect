#pragma once
/*
 * Copyright (c) 2026. Bert Laverman
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

#include <functional>


namespace SimConnect {


/**
 * Represents a registration of something identified by an ID, with an optional cleanup function
 * that is called when the registration is stopped.
 *
 * Registrations cannot be copied, but can be moved, to ensure unique ownership.
 *
 * @tparam IdType The type of the ID identifying this registration.
 */
template <class IdType>
class Registration {
    IdType id_{};                   ///< The ID identifying this registration.
    std::function<void()> cleanup_; ///< Cleanup function to be called when the registration is stopped.


    // No copies allowed
    Registration(const Registration&) = delete;
    Registration& operator=(const Registration&) = delete;


public:
    Registration() = default;
    Registration(IdType id) : id_(id) {}
    Registration(IdType id, std::function<void()> cleanup) : id_(id), cleanup_(std::move(cleanup)) {}

    Registration(Registration&&) = default;
    Registration& operator=(Registration&&) = default;

    ~Registration() {
        stop();
    }


    /**
     * Returns the ID identifying this registration.
     * @returns The ID.
     */
    [[nodiscard]]
    IdType id() const noexcept { return id_; }


    /**
     * Implicit conversion to the id.
     * @returns The ID.
     */
    operator IdType() const noexcept { return id(); }


    /**
     * Sets the cleanup function for this registration.
     *
     * @param cleanup The cleanup function to be called when the registration is stopped.
     */
    void setCleanup(std::function<void()> cleanup) {
        cleanup_ = std::move(cleanup);
    }


    /**
     * Clears the cleanup action, if any. There is no impact on the registered ID.
     */
    void clearCleanup() {
        cleanup_ = nullptr;
    }


    /**
     * Stops the registration and calls the cleanup function, if any. Safe to call more than once;
     * only the first call invokes the cleanup function.
     */
    void stop() {
        if (cleanup_) {
            auto cleanup = std::move(cleanup_);
            cleanup_ = nullptr;
            cleanup();
        }
    }
};

} // namespace SimConnect
