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

#include "pch.h"
#include <simconnect/util/logger.hpp>
#include <vector>

using namespace SimConnect;

// StringLogger implementation for testing
class StringLogger : public Logger<StringLogger> {
private:
    struct LogEntry {
        LogLevel level;
        std::string message;
    };
    
    std::vector<LogEntry> logs_;

public:
    StringLogger(std::string name = "TestLogger", LogLevel level = LogLevel::Info) 
        : Logger<StringLogger>(name, level) {}

    // This method is called by the base class Logger
    void log(LogLevel level, std::string message) {
        logs_.push_back({level, message});
    }

    const std::vector<LogEntry>& getLogs() const {
        return logs_;
    }

    void clearLogs() {
        logs_.clear();
    }

    // Convenience methods for testing
    size_t getLogCount() const {
        return logs_.size();
    }

    LogLevel getLastLogLevel() const {
        return logs_.empty() ? LogLevel::Fatal : logs_.back().level;
    }

    std::string getLastLogMessage() const {
        return logs_.empty() ? "" : logs_.back().message;
    }

    bool hasLogWithLevel(LogLevel level) const {
        for (const auto& entry : logs_) {
            if (entry.level == level) {
                return true;
            }
        }
        return false;
    }
};

// Tests for logger level filtering

TEST(LoggerTests, LoggerLevelFiltering_TraceLevel) {
    StringLogger logger("TestLogger", LogLevel::Trace);
    
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");
    logger.fatal("fatal message");
    
    EXPECT_EQ(logger.getLogCount(), 6);
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Trace));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Debug));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Info));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Warn));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Error));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Fatal));
}

TEST(LoggerTests, LoggerLevelFiltering_InfoLevel) {
    StringLogger logger("TestLogger", LogLevel::Info);
    
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");
    logger.fatal("fatal message");
    
    EXPECT_EQ(logger.getLogCount(), 4);
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Trace));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Debug));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Info));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Warn));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Error));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Fatal));
}

TEST(LoggerTests, LoggerLevelFiltering_ErrorLevel) {
    StringLogger logger("TestLogger", LogLevel::Error);
    
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");
    logger.fatal("fatal message");
    
    EXPECT_EQ(logger.getLogCount(), 2);
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Trace));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Debug));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Info));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Warn));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Error));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Fatal));
}

TEST(LoggerTests, LoggerLevelFiltering_FatalLevel) {
    StringLogger logger("TestLogger", LogLevel::Fatal);
    
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");
    logger.fatal("fatal message");
    
    EXPECT_EQ(logger.getLogCount(), 1);
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Trace));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Debug));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Info));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Warn));
    EXPECT_FALSE(logger.hasLogWithLevel(LogLevel::Error));
    EXPECT_TRUE(logger.hasLogWithLevel(LogLevel::Fatal));
}

// Tests for enabled checks

TEST(LoggerTests, EnabledChecks_TraceLevel) {
    StringLogger logger("TestLogger", LogLevel::Trace);
    
    EXPECT_TRUE(logger.isTraceEnabled());
    EXPECT_TRUE(logger.isDebugEnabled());
    EXPECT_TRUE(logger.isInfoEnabled());
    EXPECT_TRUE(logger.isWarnEnabled());
    EXPECT_TRUE(logger.isErrorEnabled());
    EXPECT_TRUE(logger.isFatalEnabled());
}

TEST(LoggerTests, EnabledChecks_InfoLevel) {
    StringLogger logger("TestLogger", LogLevel::Info);
    
    EXPECT_FALSE(logger.isTraceEnabled());
    EXPECT_FALSE(logger.isDebugEnabled());
    EXPECT_TRUE(logger.isInfoEnabled());
    EXPECT_TRUE(logger.isWarnEnabled());
    EXPECT_TRUE(logger.isErrorEnabled());
    EXPECT_TRUE(logger.isFatalEnabled());
}

TEST(LoggerTests, EnabledChecks_FatalLevel) {
    StringLogger logger("TestLogger", LogLevel::Fatal);
    
    EXPECT_FALSE(logger.isTraceEnabled());
    EXPECT_FALSE(logger.isDebugEnabled());
    EXPECT_FALSE(logger.isInfoEnabled());
    EXPECT_FALSE(logger.isWarnEnabled());
    EXPECT_FALSE(logger.isErrorEnabled());
    EXPECT_TRUE(logger.isFatalEnabled());
}

// Tests for formatted logging

TEST(LoggerTests, FormattedLogging) {
    StringLogger logger("TestLogger", LogLevel::Trace);
    
    logger.info("User {} logged in with ID {}", "John", 123);
    
    EXPECT_EQ(logger.getLogCount(), 1);
    EXPECT_EQ(logger.getLastLogLevel(), LogLevel::Info);
    EXPECT_EQ(logger.getLastLogMessage(), "User John logged in with ID 123");
}

TEST(LoggerTests, DirectLogMethod) {
    StringLogger logger("TestLogger", LogLevel::Trace);
    
    // Use the base class log method with just level and message
    logger.Logger<StringLogger>::log(LogLevel::Warn, "Direct warning message");
    
    EXPECT_EQ(logger.getLogCount(), 1);
    EXPECT_EQ(logger.getLastLogLevel(), LogLevel::Warn);
    EXPECT_EQ(logger.getLastLogMessage(), "Direct warning message");
}

TEST(LoggerTests, DirectLogMethodFormatted) {
    StringLogger logger("TestLogger", LogLevel::Trace);
    
    // Use the base class templated log method for formatting
    logger.Logger<StringLogger>::log(LogLevel::Error, "Error code: {}, description: {}", 404, "Not Found");
    
    EXPECT_EQ(logger.getLogCount(), 1);
    EXPECT_EQ(logger.getLastLogLevel(), LogLevel::Error);
    EXPECT_EQ(logger.getLastLogMessage(), "Error code: 404, description: Not Found");
}

// Test level changing

TEST(LoggerTests, ChangingLogLevel) {
    StringLogger logger("TestLogger", LogLevel::Info);
    
    logger.debug("Should not log");
    EXPECT_EQ(logger.getLogCount(), 0);
    
    logger.level(LogLevel::Debug);
    logger.debug("Should log now");
    EXPECT_EQ(logger.getLogCount(), 1);
    EXPECT_EQ(logger.getLastLogLevel(), LogLevel::Debug);
}