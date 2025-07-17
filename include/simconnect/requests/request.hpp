#pragma once
/*
 * Copyright (c) 2025. Bert Laverman
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
#include <functional>


namespace SimConnect {


/**
 * Represents a request in the SimConnect system.
 * A request is identified by an ID and can have a cleanup function that is called when the request is finished.
 * The default request ID is -1, which indicates the value isn't set or the request has finished.
 *
 * Requests cannot be copied, but can be moved, to ensure unique ownership.
 */
class Request {
    long id_{ -1l };                ///< The request ID.
    std::function<void()> cleanup_; ///< Cleanup function to be called when the request is finished.


    /**
     * Checks if the request is valid.
     * @returns True if the request is valid, false otherwise.
     */
    [[nodiscard]]
    bool valid() const noexcept { return id_ != -1l; }


    // No copies allowed
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;


public:
    Request() = default;
    Request(long id) : id_(id) {}
    Request(long id, std::function<void()> cleanup) : id_(id), cleanup_(std::move(cleanup)) {}

    Request(Request&&) = default;
    Request& operator=(Request&&) = default;

    ~Request() {
        stop();
    }


    /**
     * Returns the request ID.
     * @returns The request ID.
     */
    [[nodiscard]]
    long id() const noexcept { return id_; }


    /**
     * Implicit conversion to the id.
     * @returns The request ID.
     */
    operator long() const noexcept { return id(); }


    /**
     * Sets the cleanup function for the request.
     * @param cleanup The cleanup function to be called when the request is finished.
     */
    void set_cleanup(std::function<void()> cleanup) {
        cleanup_ = std::move(cleanup);
    }

    /**
     * Stops the request and calls the cleanup function if it is set.
     */
    void stop() {
        if (valid() && cleanup_) {
            cleanup_();
        }
    }
};

} // namespace SimConnect