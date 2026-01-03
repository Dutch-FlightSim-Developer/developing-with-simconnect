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

#include <map>
#include <tuple>
#include <array>
#include <vector>
#include <functional>


#include <simconnect/simconnect.hpp>

#include <simconnect/messaging/message_dispatcher.hpp>
#include <simconnect/simconnect_message_handler.hpp>
#include <simconnect/requests/request.hpp>


namespace SimConnect {


/**
 * The MessageHandler class provides for responsive handling of messages with correlation IDs.
 * 
 * @tparam ID The type of the correlation id.
 * @tparam D The type of the message handler, which must be derived from this class.
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 * @tparam id The MessageIds that this handler will respond to.
 */
template <class ID, class D, class M, MessageId... id>
class MessageHandler : public MessageDispatcher<ID, Messages::MsgBase, M, MultiHandlerPolicy<Messages::MsgBase>, typename M::logger_type>
{
public:
	using correlation_id_type = ID;
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
	using logger_type = typename M::logger_type;
	using handler_type = MultiHandlerPolicy<Messages::MsgBase>;
	using handler_id_type = typename handler_type::handler_id_type;
	using handler_proc_type = typename handler_type::handler_proc_type;

    using mutex_type = typename simconnect_message_handler_type::mutex_type;
    using guard_type = typename simconnect_message_handler_type::guard_type;

private:
    constexpr static size_t numIds = sizeof...(id);
    std::array<std::tuple<MessageId, handler_id_type>, numIds> registrations_;
    std::map<correlation_id_type, std::tuple<handler_type, bool>> messageHandlers_;
    std::function<void()> cleanup_;

    mutex_type mutex_;


    // No copies or moves
    MessageHandler(const MessageHandler&) = delete;
    MessageHandler(MessageHandler&&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;
    MessageHandler& operator=(MessageHandler&&) = delete;


protected:

    /**
     * Dispatches a message, if we have a handler for it.
     * 
     * @param msg The message to dispatch.
     * @param size The size of the message.
     * @returns true if we had a handler for the associated correlation ID.
     */
    [[nodiscard]]
    bool dispatch(const Messages::MsgBase& msg) {
        handler_type handler;
        bool remove{ false };

        {
            guard_type lock(mutex_);
            auto corrId = correlationId(msg);
            auto it = messageHandlers_.find(corrId);
            if (it != messageHandlers_.end()) {
                handler = std::get<0>(it->second);
                remove = std::get<1>(it->second);
            }
        }
        if (handler.hasHandlers()) {
            this->logger().debug("Dispatching to correlation ID handler for correlation ID {}", correlationId(msg));
            handler(msg);
            if (remove) {
                this->logger().debug("Auto-removing correlation ID handler for correlation ID {}", correlationId(msg));
                std::lock_guard lock(mutex_);
                messageHandlers_.erase(correlationId(msg));
            }
            return true;
        }
        else {
            this->logger().debug("No correlation ID handler for correlation ID {}", correlationId(msg));
        }
        return false;
    }


    void cleanup() {
        std::lock_guard lock(mutex_);

        if (cleanup_) {
            cleanup_();
            cleanup_ = nullptr;

            messageHandlers_.clear();
        }
    }


    /**
	 * Register the handler for the given message type ID. This also stores the original handler, so we can restore it later.
     * 
	 * @param msgHandler The message handler where we must register the handler.
	 * @param id The message type ID to register for.
     */
    void registerFor(size_t& index, simconnect_message_handler_type& msgHandler, MessageId msgId) {
        registrations_ [index++] = std::make_tuple(msgId, msgHandler.registerHandler(msgId, [this] (const Messages::MsgBase& msg) {
            if (!dispatch(msg)) {
				auto defaultHandlerProc = this->defaultHandler();
                if (defaultHandlerProc.hasHandlers()) {
                    defaultHandlerProc(msg);
                }
            }
		}));
    }


public:
    MessageHandler(std::string loggerName = "SimConnect::MessageHandler", LogLevel logLevel = LogLevel::Info)
        : MessageDispatcher<ID, Messages::MsgBase, M, MultiHandlerPolicy<Messages::MsgBase>, typename M::logger_type>(std::move(loggerName), logLevel)
    {
    }
    MessageHandler(typename M::logger_type& parentLogger, std::string loggerName = "SimConnect::MessageHandler", LogLevel logLevel = LogLevel::Info)
        : MessageDispatcher<ID, Messages::MsgBase, M, MultiHandlerPolicy<Messages::MsgBase>, typename M::logger_type>(parentLogger, std::move(loggerName), logLevel)
    {
    }
    ~MessageHandler() {
        cleanup();
    }


    /**
     * Returns the correlation ID from the message. This is not always the same field, so the actual handler class must provide it.
     * This uses the CRTP pattern to call the derived class's correlationId method.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const Messages::MsgBase& msg) {
        return static_cast<D*>(this)->correlationId(msg);
    }


    /**
     * Enable the responsive handler by registering it with the given message type ID. The current handler will be called if
     * we don't know the correlation associated with this message.
     * 
     * @param handler The handler to hook into.
     */
    void enable(simconnect_message_handler_type& msgHandler) {
        cleanup();

        std::lock_guard lock(mutex_);
        size_t regIndex{ 0 };
        (registerFor(regIndex, msgHandler, id), ...);
        cleanup_ = [this, &msgHandler]() {
            for (const auto& [registrationId, handler] : registrations_) {
                msgHandler.unRegisterHandler(registrationId, handler);
            }
        };
    }


    /**
     * Registers a handler for the given correlation ID.
     * 
     * @param correlationId The correlation ID.
     * @param correlationHandler The handler to register.
     * @param autoRemove True to automatically remove the handler after it has been called.
     */
    void registerHandler(correlation_id_type correlationId, handler_proc_type correlationHandler, bool autoRemove) {
        this->logger().debug("Registering handler for correlation ID {} (autoremove={})", correlationId, autoRemove);

        std::lock_guard lock(mutex_);

        if (!messageHandlers_.contains(correlationId)) {
            messageHandlers_.emplace(correlationId, std::make_tuple(handler_type{}, autoRemove));
		}
        std::get<0>(messageHandlers_[correlationId]).setProc(std::move(correlationHandler));
    }


    /**
     * Registers a message handler for a specific message id.
     *
     * @param id The message id.
     * @param handler The message handler's id.
     */
    void unRegisterHandler(correlation_id_type correlationId, handler_id_type handler) noexcept {
        this->logger().debug("Unregistering handler ID {} for correlation ID {}", handler, correlationId);

        std::lock_guard lock(mutex_);

        auto idHandler = this->getHandler(correlationId);

        if (idHandler.hasHandlers()) {
            idHandler.clear(handler);
		}
    }


    /**
     * Remove a registration for the given correlation ID.
     * 
     * @note If the handler has already been removed, this will do nothing.
     * 
     * @param correlationId The correlation ID.
     */
    void removeHandler(correlation_id_type correlationId) {
        std::lock_guard lock(mutex_);

        messageHandlers_.erase(correlationId);
    }

};

} // namespace SimConnect