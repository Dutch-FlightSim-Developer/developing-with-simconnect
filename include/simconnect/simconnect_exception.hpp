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
#include <format>
#include <exception>

#include <simconnect.hpp>


namespace SimConnect {



/**
 * An exception thrown by the SimConnect library.
 */
class SimConnectException : public std::exception
{
private:
	static constexpr const char* error_ = "SimConnect exception";
	const char* msg_;
public:
	SimConnectException(const char* message) : std::exception(error_), msg_(message) {}
    SimConnectException(std::string message) : std::exception(error_), msg_(_strdup(message.c_str())) {}
    SimConnectException(const char* error, const char* message) : std::exception(error), msg_(message) {}
	SimConnectException(const char* error, std::string message) : std::exception(error), msg_(_strdup(message.c_str())) {}

    const char* what() const noexcept override { return msg_; }
};


/**
 * An exception thrown when the SimConnect.cfg file does not contain the expected data.
 */
class BadConfig : public SimConnectException
{
private:
	static constexpr const char* error = "Bad SimConnect.cfg";
public:
    BadConfig(const char* message) : SimConnectException(error, std::string(error) + ": " + message) {}
	BadConfig(std::string message) : SimConnectException(error, std::string(error) + ": " + message) {}
};


/**
 * An exception thrown when an event id is unknown. Calling `SimConnect::event::get` with an unknown name will simply create a new event.
 */
class UnknownEvent : public SimConnectException
{
private:
	static constexpr const char* error = "Unknown event id";
	int id_;
public:
	UnknownEvent(int id) : SimConnectException(error, std::format("Unknown event id {}.", id).c_str()), id_(id) {}

	int id() const noexcept { return id_; }
};


/**
 * An exception thrown when a Result assertion fails (e.g., accessing value when error exists).
 */
class FailedAssertion : public SimConnectException
{
private:
	static constexpr const char* error = "Failed assertion";
public:
	FailedAssertion(const char* message) : SimConnectException(error, std::string(error) + ": " + message) {}
	FailedAssertion(std::string message) : SimConnectException(error, std::string(error) + ": " + message) {}
};

} // namespace SimConnect