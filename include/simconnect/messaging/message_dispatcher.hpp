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


#include <simconnect/util/null_logger.hpp>
#include <simconnect/messaging/handler_policy.hpp>
#include <simconnect/util/crtp.hpp>

#include <type_traits>


namespace SimConnect {


/**
 * Message dispatcher class. (using CRTP)
 * 
 * @tparam ID The message ID type.
 * @tparam M The message type.
 * @tparam D The Derived Dispatcher type.
 * @tparam H The handler policy type.
 */
template <class ID, class M, class D, class H, class L = NullLogger>
    requires std::is_base_of_v<HandlerPolicy<H, M>, H>
          && std::is_base_of_v<Logger<L>, L>
class MessageDispatcher
{
public:
    using message_id_type = ID;
    using message_type = M;
    using handler_type = H;
    using handler_proc_type = typename H::handler_proc_type;
    using handler_id_type = typename H::handler_id_type;
    using logger_type = L;


private:
    handler_type defaultHandler_;
    logger_type logger_;


    // No copies or moves
    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher(MessageDispatcher&&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(MessageDispatcher&&) = delete;

public:
    MessageDispatcher(std::string loggerName = "SimConnect::MessageDispatcher", LogLevel logLevel = LogLevel::Info)
        : defaultHandler_()
        , logger_(std::move(loggerName), logLevel)
    {
    }

    MessageDispatcher(logger_type& parentLogger, std::string loggerName = "SimConnect::MessageDispatcher", LogLevel logLevel = LogLevel::Info)
        : defaultHandler_()
        , logger_(std::move(loggerName), parentLogger, logLevel)
    {
    }

    ~MessageDispatcher() = default;
    

    /**
     * Returns the logger.
     * 
     * @returns The logger.
     */
	[[nodiscard]]
	logger_type& logger() noexcept { return logger_; }


    /**
     * Set the logger's log level.
     * 
     * @param level The log level to set.
     */
    void loggerLevel(LogLevel level) noexcept { logger_.level(level); }


    /**
     * Returns true if there is a default handler registered.
     */
    [[nodiscard]]
    bool hasDefaultHandler() const noexcept { return defaultHandler_.hasHandlers(); }


    /**
     * Returns the default message handler.
     * 
     * @returns The default message handler.
     */
    [[nodiscard]]
    handler_type defaultHandler() const noexcept { return defaultHandler_; }


    /**
     * Register a default message handler (called for unhandled message types).
     * 
     * @param handlerFunc The handler function.
     * @returns The handler ID.
     */
    handler_id_type registerDefaultHandler(handler_proc_type handlerFunc) noexcept {
        return defaultHandler_.setProc(std::move(handlerFunc));
    }


    /**
     * Returns the message handler for the specified message type.
     * 
     * @param id The message type id.
     * @returns The message handler for the specified message type.
     */
    [[nodiscard]]
    handler_type getHandler(message_id_type id) const noexcept {
        return static_cast<const D*>(this)->getHandler(id);
    }


    /**
     * Registers a message handler for a specific message id.
     *
     * @param id The message id.
     * @param handler The message handler.
     * @returns The handler id.
     */
    handler_id_type registerHandler(message_id_type id, handler_proc_type handler) noexcept {
        return static_cast<D*>(this)->registerHandlerProc(id, std::move(handler));
    }


    /**
     * Registers a message handler for a specific message id.
     *
     * @param id The message id.
     * @param handler The message handler's id.
     */
    void unRegisterHandler(message_id_type id, handler_id_type handler) noexcept {
        static_cast<D*>(this)->unRegisterHandlerProc(id, handler);
    }


    /**
     * Dispatches a SimConnect message to the correct handler.
     *
     * @param msg The message to dispatch.
     */
    void dispatch(message_id_type id, const message_type& msg) {
        auto handler = getHandler(id);
        auto defHandler = defaultHandler();

        if (handler.hasHandlers()) {
            handler(msg);
        }
        else if (defHandler.hasHandlers()) {
            defHandler(msg);
        }
        else {
            logger_.trace("No handler for message ID {}", id);
        }
    }

};

} // namespace SimConnect