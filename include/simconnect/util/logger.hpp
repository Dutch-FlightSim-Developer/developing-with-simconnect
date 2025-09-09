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


#include <string>
#include <format>


namespace SimConnect {


enum class LogLevel : size_t {
    Init = 0,
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

constexpr const char* CFG_LEVEL_INIT{ "INIT" };
constexpr const char* CFG_LEVEL_TRACE{ "TRACE" };
constexpr const char* CFG_LEVEL_DEBUG{ "DEBUG" };
constexpr const char* CFG_LEVEL_INFO{ "INFO" };
constexpr const char* CFG_LEVEL_WARN{ "WARN" };
constexpr const char* CFG_LEVEL_ERROR{ "ERROR" };
constexpr const char* CFG_LEVEL_FATAL{ "FATAL" };

constexpr const char* LogLevelNames[] = {
    CFG_LEVEL_INIT, CFG_LEVEL_TRACE, CFG_LEVEL_DEBUG, CFG_LEVEL_INFO, CFG_LEVEL_WARN, CFG_LEVEL_ERROR, CFG_LEVEL_FATAL
};


/**
 * The Logger class provides a simple logging interface.
 * 
 * It supports different log levels and can be configured to log to different outputs.
 */
template <class logger_type>
class Logger {
    std::string name_;
    LogLevel level_{ LogLevel::Info };

public:
    Logger(std::string name = "DefaultLogger", LogLevel level = LogLevel::Info) : name_(name), level_(level) {}
    ~Logger() = default;

    Logger(const Logger&) = default;
    Logger(Logger&&) = default;
    Logger& operator=(const Logger&) = default;
    Logger& operator=(Logger&&) = default;


    LogLevel level() const noexcept { return level_; }
    void level(LogLevel level) noexcept { level_ = level; }


    inline bool isTraceEnabled() { return level() <= LogLevel::Trace; }
    inline bool isDebugEnabled() { return level() <= LogLevel::Debug; }
    inline bool isInfoEnabled() { return level() <= LogLevel::Info; }
    inline bool isWarnEnabled() { return level() <= LogLevel::Warn; }
    inline bool isErrorEnabled() { return level() <= LogLevel::Error; }
    inline bool isFatalEnabled() { return level() <= LogLevel::Fatal; }

    /**
     * Logs a message at the specified log level.
     * 
     * @param level The log level to log the message at.
     * @param message The message to log.
     */
    void log(LogLevel level, std::string message) {
        static_cast<logger_type*>(this)->log(level, message);
    }


    /**
     * Logs a formatted message at the specified log level.
     * 
     * @param level The log level to log the message at.
     * @param format The format string for the message.
     * @param args The arguments to format the message with.
     */
    template<typename... Args>
    void log(LogLevel level, std::string format, Args&&... args) {
        log(level, std::vformat(format, std::make_format_args(args...)));
    }


    inline void trace(std::string txt) {
        if (isTraceEnabled()) {
            log(LogLevel::Trace, txt);
        }
    }
    template<typename... Args>
    inline void trace(std::string format, Args&&... args) {
        if (isTraceEnabled()) {
            log(LogLevel::Trace, format, std::forward<Args>(args)...);
        }
    }

    inline void debug(std::string txt) {
        if (isDebugEnabled()) {
            log(LogLevel::Debug, txt);
        }
    }
    template<typename... Args>
    inline void debug(const std::string format, Args&&... args) {
        if (isDebugEnabled()) {
            log(LogLevel::Debug, format, std::forward<Args>(args)...);
        }
    }

    inline void info(std::string txt) {
        if (isInfoEnabled()) {
            log(LogLevel::Info, txt);
        }
    }
    template<typename... Args>
    inline void info(const std::string format, Args&&... args) {
        if (isInfoEnabled()) {
            log(LogLevel::Info, format, std::forward<Args>(args)...);
        }
    }

    inline void warn(std::string txt) {
        if (isWarnEnabled()) {
            log(LogLevel::Warn, txt);
        }
    }
    template<typename... Args>
    inline void warn(const std::string format, Args&&... args) {
        if (isWarnEnabled()) {
            log(LogLevel::Warn, format, std::forward<Args>(args)...);
        }
    }

    inline void error(std::string txt) {
        if (isErrorEnabled()) {
            log(LogLevel::Error, txt);
        }
    }
    template<typename... Args>
    inline void error(const std::string format, Args&&... args) {
        if (isErrorEnabled()) {
            log(LogLevel::Error, format, std::forward<Args>(args)...);
        }
    }

    inline void fatal(std::string txt) {
        if (isFatalEnabled()) {
            log(LogLevel::Fatal, txt);
        }
    }
    template<typename... Args>
    inline void fatal(const std::string format, Args&&... args) {
        if (isFatalEnabled()) {
            log(LogLevel::Fatal, format, std::forward<Args>(args)...);
        }
    }
};

} // namespace SimConnect