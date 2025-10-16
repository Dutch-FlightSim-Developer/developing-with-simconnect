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


#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>


#include <simconnect/util/logger.hpp>


namespace SimConnect {


/**
 * The ConsoleLogger class is a specialization of Logger that logs messages to the console.
 * 
 * It inherits from Logger and implements the log method to output messages to the console.
 */
class ConsoleLogger : public Logger<ConsoleLogger> {
public:
    ConsoleLogger(std::string name = "ConsoleLogger", LogLevel level = LogLevel::Info) : Logger<ConsoleLogger>(name, level) {}
    ConsoleLogger(std::string name, ConsoleLogger& rootLogger, LogLevel level = LogLevel::Info) : Logger<ConsoleLogger>(name, rootLogger, level) {}
    ConsoleLogger(const ConsoleLogger&) = default;
    ConsoleLogger(ConsoleLogger&&) = default;
    ConsoleLogger& operator=(const ConsoleLogger&) = default;
    ConsoleLogger& operator=(ConsoleLogger&&) = default;
    ~ConsoleLogger() = default;


    void doLog(const std::string& loggerName, LogLevel level, const std::string& message) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
#if defined(_WIN32)
        localtime_s(&tm_buf, &now_c);
#else
        localtime_r(&now_c, &tm_buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        std::cerr << "[" << oss.str() << "] [" << loggerName << "] [" << LogLevelNames[static_cast<size_t>(level)] << "] " << message << std::endl;
    }
};

} // namespace SimConnect