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


#include <simconnect/connection.hpp>

#include <chrono>
#include <array>
#include <functional>


namespace SimConnect {


/**
 * A SimConnect message handler. It receives a pointer to the message and its size.
 * @see SIMCONNECT_RECV
 */
typedef std::function<void(const SIMCONNECT_RECV*, DWORD)> HandlerProc;


/**
 * A C++ concept to encapsulate a SimConnect message derived from SIMCONNECT_RECV.
 */
template <typename M>
concept DerivedFromSimConnectRecv = std::is_base_of_v<SIMCONNECT_RECV, M>;


/**
 * SimConnect message handler base class. This class uses the Curiously Recurring Template Pattern (CRTP) to prevent
 * the need for a virtual function table.
 * 
 * @tparam C The connection to handle messages from.
 * @tparam T The handler type. (this is the child extending from this base class
 */
template <class C, class T>
class Handler
{
private:
#if defined(SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST)     // MSFS 2024
    /** Array of message handlers. */
    std::array<HandlerProc, SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST+1> handlers_;
#else                                                                   // FSX
	/** Array of message handlers. */
	std::array<HandlerProc, SIMCONNECT_RECV_ID_EVENT_RACE_LAP+1> handlers_;
#endif
	/** Default message handler. */
    HandlerProc defaultHandler_;

    bool autoClosing_ = false;

protected:
	/** The connection to handle messages from. */
    C& connection_;

	/**
     * Constructor.
     * @param connection The connection to handle messages from.
     */
    Handler(C& connection) : connection_(connection) {};

public:
    ~Handler() = default;

    // No copies or moves
    Handler(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler& operator=(Handler&&) = delete;


    /**
     * @returns True if the connection will be automatically closed when the handler receives a QUIT message.
     */
    bool isAutoClosing() const noexcept { return autoClosing_; }


    /**
     * Returns the default message handler.
     * 
     * @returns The default message handler.
     */
    const HandlerProc& defaultHandler() const noexcept { return defaultHandler_; }


    /**
     * Returns the message handler for the specified message type.
     * 
     * @param id The message type id.
     * @returns The message handler for the specified message type.
     */
    const HandlerProc& getHandler(SIMCONNECT_RECV_ID id) const noexcept { return handlers_[id]; }

protected:
    /**
     * Dispatches a SimConnect message to the correct handler.
	 * @param msg The message to dispatch.
	 * @param size The size of the message.
     */
    void dispatch(const SIMCONNECT_RECV* msg, DWORD size) {
        auto& handler = handlers_[msg->dwID];
        if (handler) {
            handler(msg, size);
        }
        else {
            auto& defHandler = defaultHandler();
            if (defHandler) {
                defHandler(msg, size);
            }
        }
        // else ignore. (no specific handler, nor a default one)

        if (isAutoClosing() && msg->dwID == SIMCONNECT_RECV_ID::SIMCONNECT_RECV_ID_QUIT) {
			connection_.close();
		}
    }


    /**
     * Dispatches any waiting messages.
     */
    void dispatchWaitingMessages() {
        SIMCONNECT_RECV* msg = nullptr;
        DWORD size = 0;

        while (connection_.getNextDispatch(msg, size)) {
            dispatch(msg, size);
        }
    }

public:


    /**
     * Sets whether the connection will be automatically closed when the handler receives a QUIT message.
     * @param autoClosing True to automatically close the connection when the handler receives a QUIT message.
     */
    void autoClosing(bool autoClosing) noexcept { autoClosing_ = autoClosing; }

    /**
     * Sets the default message handler.
     * 
     * @param proc The message handler.
     */
    void setDefaultHandler(HandlerProc proc) { defaultHandler_ = proc; }


    /**
     * Registers a message handler for a specific message type.
     * 
     * @param id The message type id.
     * @param proc The message handler.
     */
    void registerHandlerProc(SIMCONNECT_RECV_ID id, HandlerProc proc) { handlers_[id] = proc; }


    /**
     * Registers a message handler for a specific message type. This is just a simple shorthand that allows you to use handlers
	 * for specific message types that are derived from `SIMCONNECT_RECV`. Note it requires a const reference to the message.
     * 
     * @tparam M The message type.
	 * @param id The message type id.
	 * @param handler The message handler.
     */
    template <DerivedFromSimConnectRecv M>
    void registerHandler(SIMCONNECT_RECV_ID id, std::function<void(const M&)> handler) {
        registerHandlerProc(id, [handler](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD size) { handler(*reinterpret_cast<const M*>(msg)); });
    }


    /**
     * Handles incoming SimConnect messages.
	 * 
	 * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void handle(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) { static_cast<T*>(this)->dispatch(duration); }
};

}
