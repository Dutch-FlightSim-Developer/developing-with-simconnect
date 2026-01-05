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
#include <optional>
#include <functional>


namespace SimConnect {


enum class LogLevel : size_t {
    Init = 0,
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Disabled
};

constexpr const char* CFG_LEVEL_INIT{ "INIT" };
constexpr const char* CFG_LEVEL_TRACE{ "TRACE" };
constexpr const char* CFG_LEVEL_DEBUG{ "DEBUG" };
constexpr const char* CFG_LEVEL_INFO{ "INFO" };
constexpr const char* CFG_LEVEL_WARN{ "WARN" };
constexpr const char* CFG_LEVEL_ERROR{ "ERROR" };
constexpr const char* CFG_LEVEL_FATAL{ "FATAL" };
constexpr const char* CFG_LEVEL_DISABLED{ "DISABLED" };

constexpr const char* LogLevelNames[] = {
    CFG_LEVEL_INIT, CFG_LEVEL_TRACE, CFG_LEVEL_DEBUG, CFG_LEVEL_INFO, CFG_LEVEL_WARN, CFG_LEVEL_ERROR, CFG_LEVEL_FATAL, CFG_LEVEL_DISABLED
};


/**
 * The Logger class provides a simple logging interface with hierarchical support.
 * 
 * It supports different log levels and can be configured to log to different outputs.
 * Non-root loggers delegate to their root logger while preserving name information.
 */
template <class logger_type, LogLevel MinimalLevel = LogLevel::Init>
class Logger {
    std::string name_;
    LogLevel level_{ LogLevel::Info };
    std::optional<std::reference_wrapper<logger_type>> rootLogger_;


public:
    Logger(std::string name = "DefaultLogger", LogLevel level = LogLevel::Info)
        : name_(std::move(name)), level_(level) {}

    Logger(std::string name, logger_type& rootLogger, LogLevel level = LogLevel::Info)
        : name_(std::move(name)), level_(level), rootLogger_(rootLogger) {}

    Logger(const Logger&) = default;
    Logger(Logger&&) = default;
    Logger& operator=(const Logger&) = default;
    Logger& operator=(Logger&&) = default;

    ~Logger() = default;

    LogLevel level() const noexcept { return level_; }
    void level(LogLevel level) noexcept { level_ = level; }
    const std::string& name() const noexcept { return name_; }
    
    bool isRootLogger() const noexcept { return !rootLogger_.has_value(); }
    void rootLogger(logger_type& rootLogger) noexcept { rootLogger_.emplace(rootLogger); }
    std::optional<std::reference_wrapper<logger_type>> getRootLogger() noexcept {
        return rootLogger_;
    }
    std::optional<std::reference_wrapper<const logger_type>> getRootLogger() const noexcept { 
        return rootLogger_; 
    }

    inline bool isTraceEnabled() { return (MinimalLevel <= LogLevel::Trace) && (level() <= LogLevel::Trace); }
    inline bool isDebugEnabled() { return (MinimalLevel <= LogLevel::Debug) && (level() <= LogLevel::Debug); }
    inline bool isInfoEnabled() { return (MinimalLevel <= LogLevel::Info) && (level() <= LogLevel::Info); }
    inline bool isWarnEnabled() { return (MinimalLevel <= LogLevel::Warn) && (level() <= LogLevel::Warn); }
    inline bool isErrorEnabled() { return (MinimalLevel <= LogLevel::Error) && (level() <= LogLevel::Error); }
    inline bool isFatalEnabled() { return (MinimalLevel <= LogLevel::Fatal) && (level() <= LogLevel::Fatal); }


    /**
     * Actual logging implementation to be provided by the derived logger_type.
     * This method is called by the base Logger class to perform the logging.
     * 
     * @param loggerName The name of the logger (for context).
     * @param level The log level of the message.
     * @param message The message to log.
     */
    void doLog(const std::string& loggerName, LogLevel logLevel, const std::string& message) {
        static_cast<logger_type*>(this)->doLog(loggerName, logLevel, message);
    }


    /**
     * Logs a message at the specified log level.
     * Non-root loggers delegate to their root logger with name information.
     * 
     * @param level The log level to log the message at.
     * @param message The message to log.
     * @param loggerName The name of the originating logger (for delegation).
     */
    void log(LogLevel logLevel, const std::string& message, const std::string& loggerName = {}) {
        if ((logLevel >= MinimalLevel) && (logLevel >= level())) {
            if (isRootLogger()) {
                // Root logger handles the actual logging
                const std::string& effectiveName = loggerName.empty() ? name_ : loggerName;
                doLog(effectiveName, logLevel, message);
            } else {
                // Non-root logger delegates to root with its own name
                rootLogger_->get().doLog(name_, logLevel, message);
            }
        }
    }

    /**
     * Logs a formatted message at the specified log level.
     * 
     * @param level The log level to log the message at.
     * @param format The format string for the message.
     * @param args The arguments to format the message with.
     */
    template<typename... Args>
    void log(LogLevel logLevel, const std::string format, Args&&... args) {
        if ((logLevel >= MinimalLevel) && (logLevel >= level())) {
            log(logLevel, std::vformat(format, std::make_format_args(args...)));
        }
    }


    inline void trace(const std::string txt) {
        if (isTraceEnabled()) {
            log(LogLevel::Trace, txt);
        }
    }
    template<typename... Args>
    inline void trace(const std::string format, Args&&... args) {
        if (isTraceEnabled()) {
            log(LogLevel::Trace, format, std::forward<Args>(args)...);
        }
    }

    inline void debug(const std::string txt) {
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

    inline void info(const std::string txt) {
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

    inline void warn(const std::string txt) {
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

    inline void error(const std::string txt) {
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

    inline void fatal(const std::string txt) {
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