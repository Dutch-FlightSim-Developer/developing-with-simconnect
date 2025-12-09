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

#include <chrono>
#include <array>
#include <functional>


#include <simconnect/connection.hpp>
#include <simconnect/messaging/handler_policy.hpp>
#include <simconnect/messaging/message_dispatcher.hpp>

#include <simconnect/util/crtp.hpp>
#include <simconnect/util/null_logger.hpp>


namespace SimConnect {


/**
 * SimConnect message handler base class. This class uses the Curiously Recurring Template Pattern (CRTP) to prevent
 * the need for a virtual function table.
 * 
 * @tparam C The connection to handle messages from.
 * @tparam M The type of the SimConnect message handler, which must derive from this class.
 * @tparam H The handler policy type.
 */
template <class C, class M, class H = MultiHandlerPolicy<SIMCONNECT_RECV>>
class SimConnectMessageHandler : public MessageDispatcher<SIMCONNECT_RECV_ID, SIMCONNECT_RECV, M, H, typename C::logger_type>
{
public:
    using connection_type = C;
    using message_handler_type = M;
    using handler_type = H;
	using handler_proc_type = typename H::handler_proc_type;
    using handler_id_type = typename H::handler_id_type;
    using logger_type = typename C::logger_type;
    using mutex_type = typename C::mutex_type;
    using guard_type = typename C::guard_type;


private:
//#if defined(SIMCONNECT_RECV_ID_FLOW_EVENT)                                                              // MSFS 2024, since SDK 1.4.0
    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_FLOW_EVENT;
//#elif defined(SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENT_PARAMS)                                          // MSFS 2020, since SDK 0.22.0
//    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENT_PARAMS;
//#elif defined(SIMCONNECT_RECV_ID_JETWAY_DATA)                                                           // MSFS 2020, since SDK 0.21.0
//    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_JETWAY_DATA;
//#elif defined(SIMCONNECT_RECV_ID_FACILITY_DATA_END)                                                     // MSFS 2020, since SDK 0.19.0
//    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_FACILITY_DATA_END;
//#elif defined(SIMCONNECT_RECV_ID_PICK)                                                                  // MSFS 2020, Experimental
//    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_PICK;
//#else                                                                                                   // FSX, since SP2
//    static constexpr SIMCONNECT_RECV_ID maxRecvId = SIMCONNECT_RECV_ID_EVENT_RACE_LAP;
//#endif

    /** Array of message handlers. */
    std::array<H, maxRecvId+1> handlers_;

    bool autoClosing_ = false;

    mutex_type mutex_;

    // No copies or moves
    SimConnectMessageHandler(const SimConnectMessageHandler&) = delete;
    SimConnectMessageHandler(SimConnectMessageHandler&&) = delete;
    SimConnectMessageHandler& operator=(const SimConnectMessageHandler&) = delete;
    SimConnectMessageHandler& operator=(SimConnectMessageHandler&&) = delete;

protected:
	/** The connection to handle messages from. */
    C& connection_;

	/**
     * Constructor.
     * @param connection The connection to handle messages from.
     */
    SimConnectMessageHandler(C& connection, std::string loggerName = "SimConnect::SimConnectMessageHandler", LogLevel logLevel = LogLevel::Info)
        : MessageDispatcher<SIMCONNECT_RECV_ID, SIMCONNECT_RECV, M, H, logger_type>(connection.logger(), std::move(loggerName), logLevel), connection_(connection)
    {
    }


public:
    ~SimConnectMessageHandler() = default;


    /**
     * @returns True if the connection will be automatically closed when the handler receives a QUIT message.
     */
    bool isAutoClosing() const noexcept { return autoClosing_; }


    /**
     * Returns the message handler for the specified message type.
     * 
     * @param id The message type id.
     * @returns The message handler for the specified message type.
     */
    [[nodiscard]]
    handler_type getHandler(SIMCONNECT_RECV_ID id) const noexcept {
        if (id < 0 || id > maxRecvId) {
            return handler_type{};
        }
        return handlers_[id];
    }



    /**
     * Dispatches a SimConnect message to the correct handler.
     */
    void dispatch(SIMCONNECT_RECV_ID id, const SIMCONNECT_RECV& msg) {
        auto handler = getHandler(id);
        auto defHandler = this->defaultHandler();
        bool shouldClose = isAutoClosing() && (id == SIMCONNECT_RECV_ID_QUIT);

        if (handler.hasHandlers()) {
            handler(msg);
        }
        else if (defHandler.hasHandlers()) {
            defHandler(msg);
        }
        else {
            this->logger().trace("No handler for message ID {}", static_cast<int>(id));
        }

        if (shouldClose) {
            connection_.close();
        }
    }

    /**
     * Dispatches a SimConnect message to the correct handler.
     *
     * @param msg The message to dispatch.
     */
    void dispatch(const SIMCONNECT_RECV* msg) {
        if (msg == nullptr) {
            this->logger().warn("Received null message to dispatch");
            return;
        }

        dispatch(static_cast<SIMCONNECT_RECV_ID>(msg->dwID), *msg);
    }


protected:
    /**
     * Dispatches any waiting messages.
     */
    void dispatchWaitingMessages() {
        volatile bool gotMessages{ false };
        while (connection_.callDispatch([this, &gotMessages](const SIMCONNECT_RECV* msg, DWORD size) {
            if (msg == nullptr) {
                this->logger().warn("Received null message from SimConnect");
                return;
            }
            if (size < msg->dwSize) {
                this->logger().warn("Received message size {} is too small for message of type {} that claims to be size {}.", size, msg->dwID, msg->dwSize);
                return;
            }
            dispatch(msg);
            gotMessages = true;
        }) && gotMessages) {
            gotMessages = false; // Keep dispatching while there are messages
        }
    }


public:

    /**
     * Returns the connection associated with this handler.
     * 
     * @returns The connection associated with this handler.
     */
    [[nodiscard]]
    C& connection() noexcept { return connection_; }


    /**
     * Sets whether the connection will be automatically closed when the handler receives a QUIT message.
     * @param autoClosing True to automatically close the connection when the handler receives a QUIT message.
     */
    void autoClosing(bool autoClosing) noexcept { autoClosing_ = autoClosing; }


    /**
     * Registers a message handler for a specific message type.
     * 
     * @param id The message type id.
     * @param proc The message handler.
     */
    handler_id_type registerHandler(SIMCONNECT_RECV_ID id, H::handler_proc_type proc) {
        guard_type guard(mutex_);

        return handlers_[id].setProc(proc);
    }


    /**
     * Registers a message handler for a specific message type.
     *
     * @param id The message type id.
     * @param proc The message handler.
     */
    void unRegisterHandler(SIMCONNECT_RECV_ID id, handler_id_type handler) {
        guard_type guard(mutex_);

        return handlers_[id].clear(handler);
    }


    /**
     * Registers a message handler for a specific message type. This is just a simple shorthand that allows you to use handlers
	 * for specific message types that are derived from `SIMCONNECT_RECV`. Note it requires a const reference to the message.
     * 
     * @tparam M The message type.
	 * @param id The message type id.
	 * @param handler The message handler.
	 * @returns The handler id.
     */
    template <class message_type>
    handler_id_type registerHandler(SIMCONNECT_RECV_ID id, std::function<void(const message_type&)> handler)
		requires(std::is_base_of<SIMCONNECT_RECV, message_type>::value)
    {
        return registerHandler(id, [handler](const SIMCONNECT_RECV& msg) { handler(*reinterpret_cast<const message_type*>(&msg)); });
    }


    /**
     * Handles incoming SimConnect messages.
	 * 
	 * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void handle(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) { static_cast<M*>(this)->dispatch(duration); }

};

}
