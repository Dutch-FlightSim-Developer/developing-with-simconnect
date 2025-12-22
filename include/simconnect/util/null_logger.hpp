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


#include <simconnect/util/logger.hpp>


namespace SimConnect{


/**
 * The NullLogger class is a logger that does not log anything.
 */
class NullLogger : public Logger<NullLogger> {
public:


    NullLogger() : Logger<NullLogger>("NullLogger", LogLevel::Disabled) {}
	NullLogger(std::string name, LogLevel level = LogLevel::Disabled) : Logger<NullLogger>(name, level) {}
	NullLogger(std::string name, NullLogger& rootLogger, LogLevel level = LogLevel::Disabled) : Logger<NullLogger>(name, rootLogger, level) {}
	NullLogger(const NullLogger&) = default;
	NullLogger(NullLogger&&) = default;
	NullLogger& operator=(const NullLogger&) = default;
	NullLogger& operator=(NullLogger&&) = default;
	~NullLogger() = default;


    void doLog([[maybe_unused]] const std::string& loggerName, [[maybe_unused]] LogLevel level, [[maybe_unused]] const std::string& message) {
        // No-op
    }
};

} // namespace SimConnect