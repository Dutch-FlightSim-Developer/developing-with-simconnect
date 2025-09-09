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

#include <map>
#include <tuple>
#include <vector>
#include <functional>


#include <simconnect/data_definition.hpp>
#include <simconnect/simconnect_message_handler.hpp>
#include <simconnect/requests/request.hpp>


namespace SimConnect {


using CorrelationHandlerType = std::function<void(const SIMCONNECT_RECV& msg)>;


/**
 * The MessageHandler class provides for responsive handling of messages with correlation IDs.
 * 
 * @tparam T The type of the message handler, which must be derived from this class.
 * @tparam id The SIMCONNECT_RECV_IDs that this handler will respond to.
 */
template <class T, SIMCONNECT_RECV_ID... id>
class MessageHandler  {
    constexpr static size_t numIds = sizeof...(id);
    std::vector<std::tuple<SIMCONNECT_RECV_ID, CorrelationHandlerType>> oldHandlers_;

    std::map<unsigned long, std::tuple<CorrelationHandlerType, bool>> messageHandlers_;
    std::function<void()> cleanup_;


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
    bool dispatch(const SIMCONNECT_RECV& msg) {
        auto corrId = correlationId(msg);
        auto it = messageHandlers_.find(corrId);
        if (it != messageHandlers_.end()) {
            auto& [handler, remove] = it->second;
            handler(msg);
            if (remove) {
                messageHandlers_.erase(it);
            }
            return true;
        }
        return false;
    }


    void cleanup() {
        if (cleanup_) {
            cleanup_();
            cleanup_ = nullptr;

            oldHandlers_.clear();
            messageHandlers_.clear();
        }
    }


    /**
	 * Register the handler for the given message type ID. This also stores the original handler, so we can restore it later.
     * 
	 * @param msgHandler The message handler where we must register the handler.
	 * @param id The message type ID to register for.
     */
    template <class simconnect_message_handler_type>
    void registerFor(simconnect_message_handler_type& msgHandler, SIMCONNECT_RECV_ID msgId) {
        auto defaultHandlerProc = msgHandler.defaultHandler();
        auto originalHandlerProc = msgHandler.getHandler(msgId);

        oldHandlers_.emplace_back(msgId, originalHandlerProc);

        msgHandler.registerHandlerProc(msgId, [this, defaultHandlerProc, originalHandlerProc] (const SIMCONNECT_RECV& msg) {
            if (!dispatch(msg)) {
                if (originalHandlerProc.proc()) {
                    originalHandlerProc(msg);
                } else if (defaultHandlerProc.proc()) {
                    defaultHandlerProc(msg);
                }
            }
		});
    }


public:
    MessageHandler() : oldHandlers_(numIds) {}
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
    unsigned long correlationId(const SIMCONNECT_RECV& msg) const {
        return static_cast<const T*>(this)->correlationId(msg);
    }


    /**
     * Enable the responsive handler by registering it with the given message type ID. The current handler will be called if
     * we don't know the correlation associated with this message.
     * 
     * @tparam ConnectionType The connection type.
     * @tparam HandlerType The handler type.
     * @param handler The handler to hook into.
     */
    template <class simconnect_message_handler_type>
    void enable(simconnect_message_handler_type& msgHandler) {
        cleanup();

        (registerFor(msgHandler, id), ...);
        cleanup_ = [this, &msgHandler]() {
            for (const auto& [id, proc] : oldHandlers_) {
                msgHandler.registerHandlerProc(id, proc);
			}
			oldHandlers_.clear();
        };
    }


    /**
     * Registers a handler for the given correlation ID.
     * 
     * @param correlationId The correlation ID.
     * @param correlationHandler The handler to register.
     * @param autoRemove True to automatically remove the handler after it has been called.
     */
    void registerHandler(unsigned long correlationId, CorrelationHandlerType correlationHandler, bool autoRemove) {
        messageHandlers_[correlationId] = std::make_tuple(correlationHandler, autoRemove);
    }


    /**
     * Remove a registration for the given correlation ID.
     * 
     * @note If the handler has already been removed, this will do nothing.
     * 
     * @param correlationId The correlation ID.
     */
    void removeHandler(unsigned long correlationId) {
        messageHandlers_.erase(correlationId);
    }

};

} // namespace SimConnect