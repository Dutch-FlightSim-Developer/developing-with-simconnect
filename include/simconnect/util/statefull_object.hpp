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


namespace SimConnect {


class StateFullObject {
    long state_{ 0 };    ///< The state of the object.

public:
    StateFullObject() = default;
    StateFullObject(const StateFullObject&) = default;
    StateFullObject(StateFullObject&&) = default;
    StateFullObject& operator=(const StateFullObject&) = default;
    StateFullObject& operator=(StateFullObject&&) = default;
    ~StateFullObject() = default;


    /**
     * Gets the current state of the object.
     * 
     * @returns The current state of the object.
     */
    [[nodiscard]]
    long state() const noexcept { return state_; }


    /**
     * Provides an implicit conversion to long to get the state of the object.
     * 
     * @returns The current state of the object.
     */
    [[nodiscard]]
    operator long() const noexcept { return state_; }


    /**
     * Sets the state of the object.
     * 
     * @param state The new state of the object.
     * @returns The new state of the object.
     */
    long state(long state) noexcept {
        state_ = state;
        return state_;
    }


    /**
     * Resets the state of the object to zero.
     * 
     * @returns The new state of the object (0).
     */
    long resetState() noexcept {
        state_ = 0;
        return state_;
    }


    /**
     * Returns if the state is good. (value equals zero)
     * 
     * @returns True if the state is good.
     */
    [[nodiscard]]
    bool succeeded() const noexcept {
        return state_ == 0;
    }


    /**
     * Provides an implicit conversion to bool to check if the state is good.
     * 
     * @returns True if the state is good.
     */
    [[nodiscard]]
    operator bool() const noexcept {
        return state_ == 0;
    }


    /**
     * Returns if the state is bad. (value less than zero)
     * 
     * @returns True if the state is bad.
     */
    [[nodiscard]]
    bool failed() const noexcept {
        return state_ < 0;
    }
};


} // namespace SimConnect