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

#include <simconnect/requests/system_state_handler.hpp>
#include <simconnect/events/system_events.hpp>

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
    WaitingForOpen,
    Connected,
    Disconnecting,
    Disconnected,
    Stopped,
    Error
};

constexpr const char* stateToString(State state) noexcept {
    switch (state) {
    case State::StartingUp: return "StartingUp";
    case State::Connecting: return "Connecting";
    case State::WaitingForOpen: return "WaitingForOpen";
    case State::Connected: return "Connected";
    case State::Disconnecting: return "Disconnecting";
    case State::Disconnected: return "Disconnected";
	case State::Stopped: return "Stopped";
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

struct VoidResult {
    bool hasValue() const noexcept { return error == ErrorCode::None; }
    ErrorCode error = ErrorCode::None;
    std::string errorMessage;
};


/**
 * Background thread manager for SimConnect connections.
 * Handles automatic connection, reconnection, and message dispatching on a background thread.
 *
 * @tparam C The connection type to use
 * @tparam H The handler type for SimConnect message processing
 */
template <class C = WindowsEventConnection<true, NullLogger>, class H = WindowsEventHandler<true, NullLogger, MultiHandlerPolicy<SIMCONNECT_RECV>>>
class BackgroundSimConnectManager {
public:
    using connection_type = C;
    using handler_type = H;
	using logger_type = typename H::logger_type;


private:
    connection_type connection_;
    handler_type handler_;

	logger_type logger_;

    bool autoConnect_{ true };
    std::chrono::milliseconds reconnectDelay_{ 3000 };
    std::chrono::milliseconds messageCheckInterval_{ 50 };
    std::chrono::milliseconds initialConnectDelay_{ 0 };
    std::chrono::milliseconds openHandshakeTimeout_{ 10000 }; // 10 seconds timeout for OPEN message
    int maxReconnectAttempts_{ -1 }; // -1 = infinite
    int configIndex_{ 0 }; // SimConnect.cfg section index

    std::atomic<State> state_{ State::Stopped };
    std::atomic<bool> shouldRun_{ false };
    std::atomic<bool> shouldConnect_{ false };
    std::atomic<bool> explicitDisconnect_{ false };
    std::atomic<int> reconnectAttempts_{ 0 };
    std::atomic<ErrorCode> lastError_{ ErrorCode::None };
    
    std::jthread workerThread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable stateCv_;
    
    StateCallback stateCallback_;
    ErrorCallback errorCallback_;
    std::string lastErrorMessage_;

    std::chrono::steady_clock::time_point openWaitStartTime_;


    // Message handler registration storage
    std::vector<std::pair<SIMCONNECT_RECV_ID, std::function<void(const SIMCONNECT_RECV&)>>> pendingHandlerRegistrations_;
    std::function<void(const SIMCONNECT_RECV&)> defaultHandlerFunc_;
    std::function<void(handler_type&)> correlationHandlerSetup_;

    // Simulator state handlers
    SystemStateHandler<handler_type> systemStateHandler_;

    // Simulator event handlers
    EventHandler<handler_type> eventHandler_;
	SystemEvents<handler_type> systemEvents_;

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
        , systemStateHandler_(handler_)
		, eventHandler_(handler_)
        , systemEvents_(eventHandler_)
    {
    }

    ~BackgroundSimConnectManager() noexcept {
        stop();
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }


#pragma region Behaviour configuration

    bool autoConnect() const noexcept { return autoConnect_; }
    BackgroundSimConnectManager<C,H>& autoConnect(bool autoConnect) noexcept { autoConnect_ = autoConnect; return *this; }


    std::chrono::milliseconds reconnectDelay() const noexcept { return reconnectDelay_; }
    BackgroundSimConnectManager<C,H>& reconnectDelay(std::chrono::milliseconds delay) noexcept { reconnectDelay_ = delay; return *this; }


    std::chrono::milliseconds messageCheckInterval() const noexcept { return messageCheckInterval_; }
    BackgroundSimConnectManager<C,H>& messageCheckInterval(std::chrono::milliseconds interval) noexcept { messageCheckInterval_ = interval; return *this; }


    std::chrono::milliseconds initialConnectDelay() const noexcept { return initialConnectDelay_; }
    BackgroundSimConnectManager<C,H>& initialConnectDelay(std::chrono::milliseconds delay) noexcept { initialConnectDelay_ = delay; return *this; }


    std::chrono::milliseconds openHandshakeTimeout() const noexcept { return openHandshakeTimeout_; }
    BackgroundSimConnectManager<C,H>& openHandshakeTimeout(std::chrono::milliseconds timeout) noexcept { openHandshakeTimeout_ = timeout; return *this; }


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

#pragma endregion

#pragma region Handler accessors

    /**
     * Return the SimConnect message handler.
     * 
     * @returns The SimConnect message handler.
     */
    [[nodiscard]]
    handler_type& simConnectHandler() noexcept {
        return handler_;
	}


    /**
     * Return the System State handler.
     * 
     * @returns The System State handler.
     */
    [[nodiscard]]
    SystemStateHandler<handler_type>& systemState() noexcept {
        return systemStateHandler_;
    }

    /**
     * Return the System Events delegate.
     * 
     * @returns The System Events delegate.
     */
    [[nodiscard]]
    SystemEvents<handler_type>& systemEvents() noexcept {
        return systemEvents_;
    }

#pragma endregion

#pragma region Control methods

    /**
     * Start the background thread.
     */
    void start() noexcept {
        {
            std::lock_guard lock(mutex_);
            if (state_.load() != State::Stopped) {
                return; // Already running
            }
        }

        shouldRun_ = true;
		transitionState(State::StartingUp);
		explicitDisconnect_ = false;

        logger_.trace("Starting worker thread...");
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
			explicitDisconnect_ = true;
        }
        cv_.notify_all();

        // Don't join here - let destructor handle it to avoid potential deadlock
    }


    /**
     * Join the worker thread (blocks until thread exits).
	 */
    void join() noexcept {
		logger_.info("Joining worker thread...");
        workerThread_.join();
	}


    /**
     * Request connection (useful when autoConnect is false or after explicit disconnect).
     */
    void connect() noexcept {
        logger_.trace("Requesting connection...");

        {
            std::lock_guard lock(mutex_);

            if (!shouldRun_) {
                logger_.trace("Not connecting because manager should shut down.");
                return;
            }
            if (state_.load() == State::Connected || state_.load() == State::Connecting || state_.load() == State::WaitingForOpen) {
                logger_.trace("Already connected or connecting; no action taken.");
                return; // Already connected or connecting
            }

            explicitDisconnect_ = false;
            reconnectAttempts_ = 0;

            if (state_.load() == State::StartingUp) {
                logger_.trace("Still starting up; connect will proceed once startup completes.");
                shouldConnect_ = true;
                return;
            }
        }
        transitionState(State::Connecting);
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

#pragma endregion

#pragma region Accessor methods

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

#pragma endregion

private:
    void workerLoop() noexcept {
        logger_.info("Worker thread started.");

        while (shouldRun_ && (state_.load() != State::Stopped)) {
            State currentState = state_.load();

            switch (currentState) {
                case State::StartingUp:
                    handleStartingUp();
                    break;
                case State::Connecting:
                    handleConnecting();
                    break;
                case State::WaitingForOpen:
                    handleWaitingForOpen();
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
                case State::Stopped:
                    // Should not happen, but just in case
                    shouldRun_ = false;
                    break;
            }
        }

		logger_.info("Worker thread exiting...");
        // Cleanup on exit
        if (state_.load() != State::Disconnected) {
			logger_.trace("Cleaning up connection on exit...");
            cleanupConnection();
        }
		transitionState(State::Stopped);
    }


#pragma region State handling

    /**
     * Helper to format version strings.
     */
    std::string version(DWORD major, DWORD minor) const noexcept {
        if (major == 0) {
            return "Unknown";
        } else if (minor == 0) {
            return std::format("{}", major);
        } else {
            return std::format("{}.{}", major, minor);
        }
	}


    /**
     * Handle StartingUp state.
     * @note This state transitions to Connecting or Disconnected.
     */
    void handleStartingUp() noexcept {
        logger_.trace("Handling StartingUp state");

        if (!shouldAttemptConnection() || !shouldRun_) {
            logger_.trace("Transitioning to Disconnected: shouldAttemptConnection()={}, shouldRun_={}", (shouldAttemptConnection() ? "true" : "false"), (shouldRun_ ? "true" : "false"));
            transitionState(State::Disconnected);
            return;
        }

        logger_.trace("Registering OPEN handler");
        // Register essential handlers for connection state management
        handler_.template registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [this](const SIMCONNECT_RECV_OPEN& msg) {
            simName_ = msg.szApplicationName;
            simVersion_ = version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor);
            simBuild_ = version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor);
            simConnectVersion_ = version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor);
            simConnectBuild_ = version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor);

            logger_.info("Connected to simulator: {} (version {}, build {}) via SimConnect version {} (build {})", 
                simName_, simVersion_, simBuild_, simConnectVersion_, simConnectBuild_);
            
            // Apply any pending handler registrations now that we're fully connected
            applyPendingHandlerRegistrations();
            
            transitionState(State::Connected);
        });
        
        logger_.trace("Registering QUIT handler");
        handler_.template registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT, [this]([[maybe_unused]]const SIMCONNECT_RECV_QUIT& msg) {
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
        
        if (shouldConnect_) {
            logger_.trace("Proceeding to connect after startup delay.");
            shouldConnect_ = false;
            transitionState(State::Connecting);
        } else if (shouldContinueRunning()) {
            transitionState(State::Connecting);
        } else {
            transitionState(State::Disconnected);
        }
    }


    /**
     * Handle Connecting state.
     * 
     * In this state, we attempt to open the SimConnect connection.
     * 
     * @note This state transitions to WaitingForOpen, Disconnected, or Error.
     */
    void handleConnecting() noexcept {
        logger_.trace("Handling Connecting state");
        if (!shouldAttemptConnection() || !shouldRun_) {
            logger_.trace("Transitioning to Disconnected: shouldAttemptConnection()={}, shouldRun_={}", (shouldAttemptConnection() ? "true" : "false"), (shouldRun_ ? "true" : "false"));
            transitionState(State::Disconnected);
            return;
        }

        // Check reconnect attempts
        if (maxReconnectAttempts_ >= 0 && reconnectAttempts_ >= maxReconnectAttempts_) {
            setError(ErrorCode::MaxReconnectAttemptsReached, "Max reconnect attempts reached");
            transitionState(State::Error);
            return;
        }

        logger_.info("Attempting to connect (attempt {})...", reconnectAttempts_ + 1);
        auto result = attemptConnection();
        if (result.hasValue()) {
            logger_.trace("Connected, will wait for OPEN handshake");
            reconnectAttempts_ = 0;
            transitionState(State::WaitingForOpen);
        } else {
            logger_.trace("Connection attempt failed: {}", result.errorMessage);

            reconnectAttempts_++;
            setError(result.error, std::format("Connection attempt {} failed: {}", reconnectAttempts_.load(), result.errorMessage));

            // Wait before retry
            {
                std::unique_lock lock(mutex_);
                cv_.wait_for(lock, reconnectDelay_, [&] { return shouldStop(); });
            }
            if (!shouldContinueRunning()) {
                logger_.trace("Giving up trying to connect, because we were asked to stop.");
                transitionState(State::Disconnected);
            }
            // Note: if shouldContinueRunning() returns true, we stay in Connecting state to retry
        }
    }


    /**
     * Handle WaitingForOpen state.
     * 
     * In this state, we wait for the SIMCONNECT_RECV_OPEN message to confirm the connection.
     * @note This state transitions to Connected, Disconnected, or Error.
     */
    void handleWaitingForOpen() noexcept {
        if (!shouldAttemptConnection() || !shouldRun_) {
            transitionState(State::Disconnecting);
            return;
        }

        // Check if connection is still valid
        if (!connection_.isOpen()) {
            setError(ErrorCode::ConnectionFailed, "Connection lost while waiting for OPEN handshake");
            transitionState(State::Disconnecting);
            return;
        }

        // Check for timeout
        auto now = std::chrono::steady_clock::now();
        if (now - openWaitStartTime_ > openHandshakeTimeout_) {
            setError(ErrorCode::ConnectionFailed, "Timeout waiting for SIMCONNECT_RECV_OPEN handshake");
            transitionState(State::Disconnecting);
            return;
        }

        // Process messages to receive the OPEN message
        auto result = processMessages();
        if (!result.hasValue()) {
            setError(result.error, result.errorMessage);
            transitionState(State::Disconnecting);
            return;
        }

        // Wait briefly before checking again
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(50), [&] { return shouldStop(); });
    }


    /**
     * Handle Connected state.
     * 
     * In this state, we continuously process messages.
     * @note This state transitions to Disconnecting on error or disconnection.
     */
    void handleConnected() noexcept {
        if (!shouldAttemptConnection() || !shouldRun_) {
            logger_.trace("Transitioning to Disconnecting: shouldAttemptConnection()={}, shouldRun_={}", (shouldAttemptConnection() ? "true" : "false"), (shouldRun_ ? "true" : "false"));
            transitionState(State::Disconnecting);
            return;
        }

        // Check if connection is still valid
        if (!connection_.isOpen()) {
            logger_.trace("Transitioning to Disconnecting: Connection lost");
            setError(ErrorCode::ConnectionFailed, "Connection lost");
            transitionState(State::Disconnecting);
            return;
        }

		logger_.trace("Processing messages...");
        // Process messages continuously with short timeout
        auto result = processMessages();
        if (!result.hasValue()) {
            setError(result.error, result.errorMessage);
            transitionState(State::Disconnecting);
            return;
        }
        
        // Wait for state change or timeout instead of blocking sleep
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, messageCheckInterval_, [&] { return shouldStop(); });
    }


    /**
     * Handle Disconnecting state.
     * 
     * In this state, we clean up the connection.
     * @note This state transitions to Disconnected.
     */
    void handleDisconnecting() noexcept {
        cleanupConnection();
        transitionState(State::Disconnected);
    }


    /**
     * Handle Disconnected state.
     * 
     * In this state, we wait for connect() calls or auto-reconnect.
     * @note This state transitions to Connecting when appropriate.
     */
    void handleDisconnected() noexcept {
        if (!shouldRun_) {
            transitionState(State::Stopped);
            return;
        }
        if (shouldAttemptConnection() && shouldRun_) {
            transitionState(State::Connecting);
        } else {
            // Wait for connect() call or stop
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1), [this] {
                return !shouldRun_ || (!explicitDisconnect_ && autoConnect_);
            });
        }
    }


    /**
     * Handle Error state.
     * 
     * In this state, we wait for manual intervention.
     * @note This state transitions to Disconnected when appropriate.
     */
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

#pragma endregion

#pragma region State support

    VoidResult attemptConnection() noexcept {
        try {
            bool success = connection_.open(configIndex_);
            
            if (success) {
                handler_.autoClosing(false); // We manage the lifecycle
                
                // Record when we started waiting for OPEN
                openWaitStartTime_ = std::chrono::steady_clock::now();
                
                return {ErrorCode::None, {}};
            } else {
                return {ErrorCode::ConnectionFailed, "Failed to open SimConnect connection"};
            }

        } catch (const std::exception& e) {
            return {ErrorCode::ResourceInitializationFailed, std::string("Connection exception: ") + e.what()};
        } catch (...) {
            return {ErrorCode::ResourceInitializationFailed, "Unknown connection error"};
        }
    }

    VoidResult processMessages() noexcept {
        try {
            handler_.handle(/*messageCheckInterval_*/);
            return {ErrorCode::None, {}};
        } catch (const std::exception& e) {
            return {ErrorCode::MessageProcessingFailed, std::string("Message processing error: ") + e.what()};
        } catch (...) {
            return {ErrorCode::MessageProcessingFailed, "Unknown message processing error"};
        }
    }

    void applyPendingHandlerRegistrations() noexcept {
        try {
            // Apply default handler
            if (defaultHandlerFunc_) {
                [[maybe_unused]] auto defaultHandlerId = handler_.registerDefaultHandler(defaultHandlerFunc_);
            }
            
            // Apply message type handlers
            for (const auto& [messageId, handlerFunc] : pendingHandlerRegistrations_) {
                [[maybe_unused]] auto handlerId = handler_.registerHandler(messageId, handlerFunc);
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

#pragma endregion

#pragma region State queries

    bool shouldAttemptConnection() const noexcept {
        return autoConnect_ && !explicitDisconnect_;
    }

    bool shouldContinueRunning() const noexcept {
        return shouldRun_ && !explicitDisconnect_;
    }

    bool shouldStop() const noexcept {
        return !shouldRun_ || explicitDisconnect_;
	}

#pragma endregion

#pragma region State housekeeping

    void transitionState(State newState) noexcept {
        State oldState = state_.exchange(newState);

        if (oldState != newState) {
            notifyStateChange(newState, oldState);
			stateCv_.notify_all();
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

#pragma endregion

public:

    /**
     * Wait for the state to change to the desired state.
     * 
     * @param desiredState The desired state to wait for.
     * @param timeout The maximum time to wait for the state change.
     * @returns True if the state changed to the desired state, false if the timeout was reached.
     */
    bool waitForState(State desiredState, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) noexcept {
		auto deadline = std::chrono::steady_clock::now() + timeout;
		auto milliSecondsRemaining = timeout;

        while ((state_.load() != desiredState) && (milliSecondsRemaining.count() > 0)) {
            std::unique_lock lock(mutex_);
            stateCv_.wait_for(lock, milliSecondsRemaining, [&] { return getState() == desiredState; });
			milliSecondsRemaining = std::chrono::milliseconds((deadline - std::chrono::steady_clock::now()).count());
		}

        return getState() == desiredState;
    }
};

} // namespace SimConnect