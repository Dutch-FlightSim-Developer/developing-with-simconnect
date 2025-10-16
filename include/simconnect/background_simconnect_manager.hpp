#pragma once
/*
 * Copyright (c) 2024, 2025. Bert Laverman
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

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <optional>

namespace SimConnect {


/**
 * Helper function to convert state to string.
 */
enum class State {
    StartingUp,
    Connecting,
    Connected,
    Disconnecting,
    Disconnected,
    Error
};

constexpr const char* stateToString(State state) noexcept {
    switch (state) {
    case State::StartingUp: return "StartingUp";
    case State::Connecting: return "Connecting";
    case State::Connected: return "Connected";
    case State::Disconnecting: return "Disconnecting";
    case State::Disconnected: return "Disconnected";
    case State::Error: return "Error";
    default: return "Unknown";
    }
}

enum class ErrorCode {
    None,
    ConnectionFailed,
    MessageProcessingFailed,
    MaxReconnectAttemptsReached,
    InvalidState,
    ResourceInitializationFailed
};

/**
 * Helper function to convert error code to string.
 */
constexpr const char* errorCodeToString(ErrorCode error) noexcept {
    switch (error) {
    case ErrorCode::None: return "None";
    case ErrorCode::ConnectionFailed: return "ConnectionFailed";
    case ErrorCode::MessageProcessingFailed: return "MessageProcessingFailed";
    case ErrorCode::MaxReconnectAttemptsReached: return "MaxReconnectAttemptsReached";
    case ErrorCode::InvalidState: return "InvalidState";
    case ErrorCode::ResourceInitializationFailed: return "ResourceInitializationFailed";
    default: return "Unknown";
    }
}

using StateCallback = std::function<void(State newState, State oldState)>;
using ErrorCallback = std::function<void(ErrorCode errorCode, const std::string& errorMessage)>;

template<typename T>
struct Result {
    std::optional<T> value;
    ErrorCode error = ErrorCode::None;
    std::string errorMessage;

    bool hasValue() const noexcept { return value.has_value(); }
    bool hasError() const noexcept { return error != ErrorCode::None; }
    
    T& operator*() noexcept { return *value; }
    const T& operator*() const noexcept { return *value; }
    T* operator->() noexcept { return &(*value); }
    const T* operator->() const noexcept { return &(*value); }
};

using VoidResult = Result<std::monostate>;


/**
 * Background thread manager for SimConnect connections.
 * Handles automatic connection, reconnection, and message dispatching on a background thread.
 *
 * @tparam C The connection type to use
 * @tparam H The handler type for SimConnect message processing
 */
template <class C = WindowsEventConnection<true, NullLogger>, class H = WindowsEventHandler<true, NullLogger, MultiHandlerPolicy<>>>
class BackgroundSimConnectManager {
public:
    using connection_type = typename C;
    using handler_type = typename H;
	using logger_type = typename H::logger_type;


private:
    connection_type connection_;
    handler_type handler_;

	logger_type logger_;

    bool autoConnect_{ true };
    std::chrono::milliseconds reconnectDelay_{ 3000 };
    std::chrono::milliseconds messageCheckInterval_{ 50 };
    std::chrono::milliseconds initialConnectDelay_{ 0 };
    int maxReconnectAttempts_{ -1 }; // -1 = infinite
    int configIndex_{ 0 }; // SimConnect.cfg section index

    std::atomic<State> state_{ State::StartingUp };
    std::atomic<bool> shouldRun_{ false };
    std::atomic<bool> explicitDisconnect_{ false };
    std::atomic<int> reconnectAttempts_{ 0 };
    std::atomic<ErrorCode> lastError_{ ErrorCode::None };
    
    std::jthread workerThread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    StateCallback stateCallback_;
    ErrorCallback errorCallback_;
    std::string lastErrorMessage_;


    // Message handler registration storage
    std::vector<std::pair<SIMCONNECT_RECV_ID, std::function<void(const SIMCONNECT_RECV&)>>> pendingHandlerRegistrations_;
    std::function<void(const SIMCONNECT_RECV&)> defaultHandlerFunc_;
    std::function<void(handler_type&)> correlationHandlerSetup_;


    // Simulator information
    std::string simName_{};
    std::string simVersion_{};
    std::string simBuild_{};
    std::string simConnectVersion_{};
    std::string simConnectBuild_{};


    // Non-copyable, non-movable
    BackgroundSimConnectManager(const BackgroundSimConnectManager&) = delete;
    BackgroundSimConnectManager(BackgroundSimConnectManager&&) = delete;
    BackgroundSimConnectManager& operator=(const BackgroundSimConnectManager&) = delete;
    BackgroundSimConnectManager& operator=(BackgroundSimConnectManager&&) = delete;

public:
    explicit BackgroundSimConnectManager(std::string clientName, int configIndex = 0) noexcept
        : connection_(std::move(clientName))
        , handler_(connection_)
        , logger_("SimConnect::BackgroundSimConnectManager", connection_.logger())
        , configIndex_(configIndex)
    {
    }

    ~BackgroundSimConnectManager() noexcept {
        stop();
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }


    bool autoConnect() const noexcept { return autoConnect_; }
    BackgroundSimConnectManager<C,H>& autoConnect(bool autoConnect) noexcept { autoConnect_ = autoConnect; return *this; }


    std::chrono::milliseconds reconnectDelay() const noexcept { return reconnectDelay_; }
    BackgroundSimConnectManager<C,H>& reconnectDelay(std::chrono::milliseconds delay) noexcept { reconnectDelay_ = delay; return *this; }


    std::chrono::milliseconds messageCheckInterval() const noexcept { return messageCheckInterval_; }
    BackgroundSimConnectManager<C,H>& messageCheckInterval(std::chrono::milliseconds interval) noexcept { messageCheckInterval_ = interval; return *this; }


    std::chrono::milliseconds initialConnectDelay() const noexcept { return initialConnectDelay_; }
    BackgroundSimConnectManager<C,H>& initialConnectDelay(std::chrono::milliseconds delay) noexcept { initialConnectDelay_ = delay; return *this; }


    int maxReconnectAttempts() const noexcept { return maxReconnectAttempts_; }
    BackgroundSimConnectManager<C,H>& maxReconnectAttempts(int attempts) noexcept { maxReconnectAttempts_ = attempts; return *this; }


    int configIndex() const noexcept { return configIndex_; }
    BackgroundSimConnectManager<C,H>& configIndex(int index) noexcept { configIndex_ = index; return *this; }

    BackgroundSimConnectManager<C,H>& logLevel(LogLevel level) noexcept { 
        std::lock_guard lock(mutex_);
        logger_.level(level); 
        return *this;
    }
    BackgroundSimConnectManager<C,H>& connectionLogLevel(LogLevel level) noexcept { 
        std::lock_guard lock(mutex_);
        connection_.logger().level(level); 
        return *this;
    }
    BackgroundSimConnectManager<C,H>& handlerLogLevel(LogLevel level) noexcept { 
        std::lock_guard lock(mutex_);
        handler_.logger().level(level); 
        return *this;
    }

    /**
     * Start the background thread.
     */
    void start() noexcept {
        std::lock_guard lock(mutex_);
        if (shouldRun_) {
            return; // Already running
        }

        shouldRun_ = true;
        workerThread_ = std::jthread(&BackgroundSimConnectManager::workerLoop, this);
    }

    /**
     * Stop the background thread and disconnect.
     */
    void stop() noexcept {
        {
            std::lock_guard lock(mutex_);
            if (!shouldRun_) {
                return; // Already stopped
            }
            shouldRun_ = false;
        }
        cv_.notify_all();

        // Don't join here - let destructor handle it to avoid potential deadlock
    }

    /**
     * Request connection (useful when autoConnect is false or after explicit disconnect).
     */
    void connect() noexcept {
        std::lock_guard lock(mutex_);
        explicitDisconnect_ = false;
        reconnectAttempts_ = 0;
        cv_.notify_all();
    }

    /**
     * Request disconnection (disables auto-reconnect until connect() is called).
     */
    void disconnect() noexcept {
        std::lock_guard lock(mutex_);
        explicitDisconnect_ = true;
        cv_.notify_all();
    }

    /**
     * Get current state.
     */
    [[nodiscard]]
    State getState() const noexcept {
        return state_.load();
    }

    /**
     * Check if connected.
     */
    [[nodiscard]]
    bool isConnected() const noexcept {
        return state_.load() == State::Connected;
    }

    /**
     * Get last error information.
     */
    [[nodiscard]]
    ErrorCode getLastError() const noexcept {
        return lastError_.load();
    }

    /**
     * Get last error message (thread-safe).
     */
    [[nodiscard]]
    std::string getLastErrorMessage() const noexcept {
        std::lock_guard lock(mutex_);
        return lastErrorMessage_;
    }

    /**
     * Get the underlying connection.
     * The connection object always exists, its methods handle disconnected state gracefully.
     */
    [[nodiscard]]
    operator connection_type&() noexcept {
        return connection_;
    }

    /**
     * Get the underlying connection (const version).
     */
    [[nodiscard]]
    operator const connection_type&() const noexcept {
        return connection_;
    }

    /**
     * Set state change callback.
     */
    void setStateCallback(StateCallback callback) noexcept {
        std::lock_guard lock(mutex_);
        stateCallback_ = std::move(callback);
    }

    /**
     * Set error callback.
     */
    void setErrorCallback(ErrorCallback callback) noexcept {
        std::lock_guard lock(mutex_);
        errorCallback_ = std::move(callback);
    }

    /**
     * Register a message handler for a specific message type.
     * Safe to call whether connected or not - will apply when connection is established.
     */
    template<SIMCONNECT_RECV_ID MessageId>
    void registerMessageHandler(std::function<void(const SIMCONNECT_RECV&)> handlerFunc) noexcept {
        std::lock_guard lock(mutex_);
        pendingHandlerRegistrations_.emplace_back(MessageId, std::move(handlerFunc));
        
        // Apply immediately if connected
        if (isConnected()) {
            try {
                handler_.template registerHandlerProc<MessageId>(pendingHandlerRegistrations_.back().second);
            } catch (...) {
                // Ignore registration errors - will retry on next connection
            }
        }
    }

    /**
     * Register a default message handler (called for unhandled message types).
     */
    void registerDefaultMessageHandler(std::function<void(const SIMCONNECT_RECV&)> handlerFunc) noexcept {
        std::lock_guard lock(mutex_);
        defaultHandlerFunc_ = std::move(handlerFunc);
        
        // Apply immediately if connected
        if (isConnected()) {
            try {
                handler_.setDefaultHandler(defaultHandlerFunc_);
            } catch (...) {
                // Ignore registration errors - will retry on next connection
            }
        }
    }

    /**
     * Register a correlation-based message handler.
     * This requires a MessageHandler template to be used with the handler.
     */
    template<typename MessageHandlerType>
    void enableCorrelationHandler(MessageHandlerType& correlationHandler) noexcept {
        std::lock_guard lock(mutex_);
        
        // Store for later application
        correlationHandlerSetup_ = [&correlationHandler](auto& handler) {
            try {
                correlationHandler.enable(handler);
            } catch (...) {
                // Ignore setup errors
            }
        };
        
        // Apply immediately if connected
        if (isConnected() && correlationHandlerSetup_) {
            correlationHandlerSetup_(handler_);
        }
    }

    /**
     * Clear all registered message handlers.
     */
    void clearMessageHandlers() noexcept {
        std::lock_guard lock(mutex_);
        pendingHandlerRegistrations_.clear();
        defaultHandlerFunc_ = nullptr;
        correlationHandlerSetup_ = nullptr;
    }

private:
    void workerLoop() noexcept {
        while (shouldRun_) {
            State currentState = state_.load();

            switch (currentState) {
                case State::StartingUp:
                    handleStartingUp();
                    break;
                case State::Connecting:
                    handleConnecting();
                    break;
                case State::Connected:
                    handleConnected();
                    break;
                case State::Disconnecting:
                    handleDisconnecting();
                    break;
                case State::Disconnected:
                    handleDisconnected();
                    break;
                case State::Error:
                    handleError();
                    break;
            }
        }

        // Cleanup on exit
        if (state_.load() != State::Disconnected) {
            cleanupConnection();
        }
    }

    inline std::string version(DWORD major, DWORD minor) const noexcept {
        if (major == 0) {
            return "Unknown";
        } else if (minor == 0) {
            return std::format("{}", major);
        } else {
            return std::format("{}.{}", major, minor);
        }
	}

    void handleStartingUp() noexcept {
        if (!shouldAttemptConnection() || !shouldRun_) {
            transitionState(State::Disconnected);
            return;
        }

        handler_.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [this](const SIMCONNECT_RECV_OPEN& msg) {
            simName_ = msg.szApplicationName;
			simVersion_ = version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor);
			simBuild_ = version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor);
			simConnectVersion_ = version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor);
			simConnectBuild_ = version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor);

            logger_.info("Connected to simulator: {} (version {}, build {}) via SimConnect version {} (build {})", 
                simName_, simVersion_, simBuild_, simConnectVersion_, simConnectBuild_);
		});
        handler_.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, [this]([[maybe_unused]] const SIMCONNECT_RECV_QUIT& msg) {
            logger_.warn("Received QUIT message from simulator");
            setError(ErrorCode::ConnectionFailed, "Simulator quit");
			transitionState(State::Disconnecting);
		});
        logger_.info("Starting up with initial connect delay of {} ms", initialConnectDelay_.count());
        
        // Apply initial delay for autoConnect startup
        if (initialConnectDelay_.count() > 0) {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, initialConnectDelay_, [&] { return shouldStop(); });
        }
        
        if (shouldContinueRunning()) {
            transitionState(State::Connecting);
        } else {
            transitionState(State::Disconnected);
        }
    }

    void handleConnecting() noexcept {
        if (!shouldAttemptConnection() || !shouldRun_) {
            transitionState(State::Disconnected);
            return;
        }

        // Check reconnect attempts
        if (maxReconnectAttempts_ >= 0 && reconnectAttempts_ >= maxReconnectAttempts_) {
            setError(ErrorCode::MaxReconnectAttemptsReached, "Max reconnect attempts reached");
            transitionState(State::Error);
            return;
        }

        auto result = attemptConnection();
        if (result.hasValue()) {
            reconnectAttempts_ = 0;
            transitionState(State::Connected);
        } else {
            reconnectAttempts_++;
            setError(result.error, std::format("Connection attempt {} failed: {}", reconnectAttempts_.load(), result.errorMessage));

            // Wait before retry
            {
                std::unique_lock lock(mutex_);
                cv_.wait_for(lock, reconnectDelay_, [&] { return shouldStop(); });
            }
            if (!shouldContinueRunning()) {
                transitionState(State::Disconnected);
            }
            // Note: if shouldContinueRunning() returns true, we stay in Connecting state to retry
        }
    }

    void handleConnected() noexcept {
        if (!shouldAttemptConnection() || !shouldRun_) {
            transitionState(State::Disconnecting);
            return;
        }

        // Check if connection is still valid
        if (!connection_.isOpen()) {
            setError(ErrorCode::ConnectionFailed, "Connection lost");
            transitionState(State::Disconnecting);
            return;
        }

        // Process messages continuously with short timeout
        auto result = processMessages();
        if (result.hasError()) {
            setError(result.error, result.errorMessage);
            transitionState(State::Disconnecting);
            return;
        }
        
        // Wait for state change or timeout instead of blocking sleep
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, messageCheckInterval_, [&] { return shouldStop(); });
    }

    void handleDisconnecting() noexcept {
        cleanupConnection();
        transitionState(State::Disconnected);
    }

    void handleDisconnected() noexcept {
        if (shouldAttemptConnection() && shouldRun_) {
            //// Check if this is initial startup with autoConnect
            //if (autoConnect_ && initialConnectDelay_.count() > 0) {
            //    transitionState(State::StartingUp);
            //} else {
                transitionState(State::Connecting);
            //}
        } else {
            // Wait for connect() call or stop
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return !shouldRun_ || (!explicitDisconnect_ && autoConnect_);
            });
        }
    }

    void handleError() noexcept {
        // Wait in error state until manual intervention
        {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(5), [this] {
                return !shouldRun_ || !explicitDisconnect_;
            });
        }        
        if (shouldContinueRunning()) {
            // Reset and try again if autoConnect is enabled
            reconnectAttempts_ = 0;
            transitionState(State::Disconnected);
        }
    }

    VoidResult attemptConnection() noexcept {
        try {
            bool success = connection_.open(configIndex_);
            
            if (success) {
                handler_.autoClosing(false); // We manage the lifecycle
                
                // Apply any pending handler registrations
                applyPendingHandlerRegistrations();
                
                return {std::monostate{}, ErrorCode::None, {}};
            } else {
                return {std::nullopt, ErrorCode::ConnectionFailed, "Failed to open SimConnect connection"};
            }

        } catch (const std::exception& e) {
            return {std::nullopt, ErrorCode::ResourceInitializationFailed, std::string("Connection exception: ") + e.what()};
        } catch (...) {
            return {std::nullopt, ErrorCode::ResourceInitializationFailed, "Unknown connection error"};
        }
    }

    VoidResult processMessages() noexcept {
        try {
            handler_.handle(messageCheckInterval_);
            return {std::monostate{}, ErrorCode::None, {}};
        } catch (const std::exception& e) {
            return {std::nullopt, ErrorCode::MessageProcessingFailed, std::string("Message processing error: ") + e.what()};
        } catch (...) {
            return {std::nullopt, ErrorCode::MessageProcessingFailed, "Unknown message processing error"};
        }
    }

    void applyPendingHandlerRegistrations() noexcept {
        try {
            // Apply default handler
            if (defaultHandlerFunc_) {
                handler_.setDefaultHandler(defaultHandlerFunc_);
            }
            
            // Apply message type handlers
            for (const auto& [messageId, handlerFunc] : pendingHandlerRegistrations_) {
                handler_.registerHandlerProc(messageId, handlerFunc);
            }
            
            // Apply correlation handler setup
            if (correlationHandlerSetup_) {
                correlationHandlerSetup_(handler_);
            }
            
        } catch (...) {
            // Ignore application errors - handlers will be retried on next connection
        }
    }

    void cleanupConnection() noexcept {
        try {
            if (connection_.isOpen()) {
                connection_.close();
            }
        } catch (...) {
            // Ignore exceptions during cleanup
        }
    }

    bool shouldAttemptConnection() const noexcept {
        return autoConnect_ && !explicitDisconnect_;
    }

    bool shouldContinueRunning() const noexcept {
        return shouldRun_ && !explicitDisconnect_;
    }

    bool shouldStop() const noexcept {
        return !shouldRun_ || explicitDisconnect_;
	}

    void transitionState(State newState) noexcept {
        State oldState = state_.exchange(newState);

        if (oldState != newState) {
            notifyStateChange(newState, oldState);
        }
    }

    void setError(ErrorCode error, const std::string& message) noexcept {
        lastError_ = error;
        {
            std::lock_guard lock(mutex_);
            lastErrorMessage_ = message;
        }
        notifyError(error, message);
    }

    void notifyStateChange(State newState, State oldState) noexcept {
		logger_.info("Transitioned from {} to {}", stateToString(oldState), stateToString(newState));

        StateCallback callback;
        {
            std::lock_guard lock(mutex_);
            callback = stateCallback_;
        }
        
        if (callback) {
            try {
                callback(newState, oldState);
            } catch (...) {
                // Ignore callback exceptions
            }
        }
    }

    void notifyError(ErrorCode error, const std::string& message) noexcept {
        logger_.error(message);

        ErrorCallback callback;
        {
            std::lock_guard lock(mutex_);
            callback = errorCallback_;
        }
        
        if (callback) {
            try {
                callback(error, message);
            } catch (...) {
                // Ignore callback exceptions
            }
        }
    }
};

} // namespace SimConnect