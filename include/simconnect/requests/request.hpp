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

#include <simconnect/requests/requests.hpp>
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
    RequestId id_{ 0 };                ///< The request ID.
    std::function<void()> cleanup_; ///< Cleanup function to be called when the request is finished.


    /**
     * Checks if the request is valid.
     * @returns True if the request is valid, false otherwise.
     */
    [[nodiscard]]
    bool valid() const noexcept { return id_ != 0; }


    // No copies allowed
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;


public:
    Request() = default;
    Request(RequestId id) : id_(id) {}
    Request(RequestId id, std::function<void()> cleanup) : id_(id), cleanup_(std::move(cleanup)) {}

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
    RequestId id() const noexcept { return id_; }


    /**
     * Implicit conversion to the id.
     * @returns The request ID.
     */
    operator RequestId() const noexcept { return id(); }


    /**
     * Sets the cleanup function for the request.
     * 
     * @param cleanup The cleanup function to be called when the request is finished.
     */
    void setCleanup(std::function<void()> cleanup) {
        cleanup_ = std::move(cleanup);
    }


    /**
     * Clears the cleanup action, if any. There is no impact on the registered request ID.
     */
    void clearCleanup() {
        cleanup_ = nullptr;
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