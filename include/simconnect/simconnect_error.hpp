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


#include <string>
#include <variant>


namespace SimConnect {

/**
 * An error returned by SimConnect functions.
 */
class Error {
    long hr_;
    std::string message_;

public:
    constexpr Error() : hr_(0), message_("") {}
    constexpr Error(long hr, std::string message) : hr_(hr), message_(std::move(message)) {}
    constexpr Error(const Error&) = default;
    constexpr Error(Error&&) = default;
    constexpr Error& operator=(const Error&) = default;
    constexpr Error& operator=(Error&&) = default;
    constexpr ~Error() = default;
    [[nodiscard]]
    constexpr long hr() const noexcept { return hr_; }
    [[nodiscard]]
    constexpr const std::string& message() const noexcept { return message_; }

    [[nodiscard]]
    constexpr bool ok() const noexcept { return hr_ == 0; }
    [[nodiscard]]
    constexpr bool failed() const noexcept { return hr_ != 0; }
    [[nodiscard]]
    constexpr operator bool() const noexcept { return hr_ == 0; }
};


/**
 * A Result type that can hold either a value of type T or an Error.
 * 
 * Provides convenient accessors and implicit conversion to T when successful.
 * 
 * @tparam T The type of the successful result.
 */
template <class T>
class Result {
    std::variant<T, Error> value_;

public:
    // Constructors
    constexpr Result(T value) : value_(value) {}
    constexpr Result(Error error) : value_(std::move(error)) {}
    constexpr Result(T value, long hr, std::string message) { 
        if (hr == 0) {
            value_ = std::move(value);
        } else {
            value_ = Error(hr, std::move(message));
        }
    }
    constexpr Result(const Result&) = default;
    constexpr Result(Result&&) = default;
    constexpr Result& operator=(const Result&) = default;
    constexpr Result& operator=(Result&&) = default;
    constexpr ~Result() = default;

    // Check if result contains a value
    [[nodiscard]]
    constexpr bool hasValue() const noexcept {
        return std::holds_alternative<T>(value_);
    }

    // Check if result contains an error
    [[nodiscard]]
    constexpr bool hasError() const noexcept {
        return std::holds_alternative<Error>(value_);
    }

    // Boolean conversion - true if successful
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept {
        return hasValue();
    }

    // Implicit conversion to T (throws if error)
    constexpr operator T() const {
        if (hasError()) {
            throw FailedAssertion("Result contains error: " + error().message());
        }
        return std::get<T>(value_);
    }

    // Get the value (throws if error)
    [[nodiscard]]
    constexpr T& value() {
        if (hasError()) {
            throw FailedAssertion("Result contains error: " + error().message());
        }
        return std::get<T>(value_);
    }

    [[nodiscard]]
    constexpr const T& value() const {
        if (hasError()) {
            throw FailedAssertion("Result contains error: " + error().message());
        }
        return std::get<T>(value_);
    }

    // Get the error (throws if value)
    [[nodiscard]]
    constexpr Error& error() {
        if (hasValue()) {
            throw FailedAssertion("Result contains value, not error");
        }
        return std::get<Error>(value_);
    }

    [[nodiscard]]
    constexpr const Error& error() const {
        if (hasValue()) {
            throw FailedAssertion("Result contains value, not error");
        }
        return std::get<Error>(value_);
    }

    // Get value or default
    [[nodiscard]]
    constexpr T valueOr(T defaultValue) const {
        return hasValue() ? std::get<T>(value_) : std::move(defaultValue);
    }

    // Access underlying variant
    [[nodiscard]]
    constexpr const std::variant<T, Error>& variant() const noexcept {
        return value_;
    }
};

} // namespace SimConnect