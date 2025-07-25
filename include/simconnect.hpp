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

#pragma warning(push, 3)
#include <Windows.h>
#include <SimConnect.h>

// Handle some 2020 vs 2024 differences in the SimConnect.h header.
#if !defined(SIMCONNECT_OBJECT_ID_USER_AICRAFT)
#define SIMCONNECT_OBJECT_ID_USER_AICRAFT SIMCONNECT_OBJECT_ID_USER
#endif
#if !defined(SIMCONNECT_OBJECT_ID_USER_AVATAR)
#define SIMCONNECT_OBJECT_ID_USER_AVATAR SIMCONNECT_OBJECT_ID_USER
#endif
#if !defined(SIMCONNECT_OBJECT_ID_USER_CURRENT)
#define SIMCONNECT_OBJECT_ID_USER_CURRENT SIMCONNECT_OBJECT_ID_USER
#endif

#pragma warning(pop)

#include <exception>
#include <format>
#include <string>
#include <functional>


namespace SimConnect
{


/**
 * An exception thrown by the SimConnect library.
 */
class SimConnectException : public std::exception
{
private:
	static constexpr const char* error_ = "SimConnect exception";
	std::string msg_;
public:
	SimConnectException(const char* message) : std::exception(error_), msg_(message) {}
	SimConnectException(const char* error, std::string message) : std::exception(error), msg_(message) {}
	const char* what() const noexcept override { return msg_.c_str(); }
};


/**
 * An exception thrown when the SimConnect.cfg file does not contain the expected data.
 */
class BadConfig : public SimConnectException
{
private:
	static constexpr const char* error = "Bad SimConnect.cfg";
public:
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
 * This wraps a lambda to solve ambiguity issues, when you have both void(bool) and void(int) or some such overloads.
 * 
 * @tparam T The parameter type. This should be the only one you must explicitly name.
 * @tparam F The actual Lambda or function pointer type.
 * @return An explicitly cast std::function<void(T)> value.
 */
template<typename T, typename F>
std::function<void(T)> wrap(F&& f) {
	return std::function<void(T)>(std::forward<F>(f));
}

}