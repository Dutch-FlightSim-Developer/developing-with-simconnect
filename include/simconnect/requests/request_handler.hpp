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
#include <array>
#include <vector>
#include <functional>


#include <simconnect/data_definition.hpp>
#include <simconnect/handler.hpp>
#include <simconnect/requests/request.hpp>


namespace SimConnect {

/**
 * The RequestHandler class provides for responsive handling of requests.
 * 
 * @tparam T The type of the request handler, which must be derived from this class.
 * @tparam id The SIMCONNECT_RECV_IDs that this handler will respond to.
 */
template <class T, SIMCONNECT_RECV_ID... id>
class RequestHandler  {
    constexpr static size_t numIds = sizeof...(id);
    std::vector<std::tuple<SIMCONNECT_RECV_ID, HandlerProc>> oldHandlers_;

    std::map<unsigned long, std::tuple<HandlerProc, bool>> requestHandlers_;
    std::function<void()> cleanup_;


    // No copies or moves
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler(RequestHandler&&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    RequestHandler& operator=(RequestHandler&&) = delete;


protected:

    /**
     * Dispatches a message, if we have a handler for it.
     * 
     * @param msg The message to dispatch.
     * @param size The size of the message.
     * @returns true if we had a handler for the associated request ID.
     */
    [[nodiscard]]
    bool dispatch(const SIMCONNECT_RECV* msg, DWORD size) {
        auto reqId = requestId(msg);
        auto it = requestHandlers_.find(reqId);
        if (it != requestHandlers_.end()) {
            auto& [handler, remove] = it->second;
            handler(msg, size);
            if (remove) {
                requestHandlers_.erase(it);
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
            requestHandlers_.clear();
        }
    }


    /**
	 * Register the handler for the given message type ID. This also stores the original handler, so we can restore it later.
     * 
	 * @param msgHandler The message handler where we must register the handler.
	 * @param id The message type ID to register for.
     */
    template <class ConnectionType, class HandlerType>
    void registerFor(Handler<ConnectionType, HandlerType>& msgHandler, SIMCONNECT_RECV_ID msgId) {
        auto defaultHandlerProc = msgHandler.defaultHandler();
        auto originalHandlerProc = msgHandler.getHandler(msgId);

        oldHandlers_.emplace_back(msgId, originalHandlerProc);

        msgHandler.registerHandlerProc(msgId, [this, defaultHandlerProc, originalHandlerProc] (const SIMCONNECT_RECV* msg, DWORD size) {
            if (!dispatch(msg, size)) {
                if (originalHandlerProc) {
                    originalHandlerProc(msg, size);
                } else if (defaultHandlerProc) {
                    defaultHandlerProc(msg, size);
                }
            }
		});
    }


public:
    RequestHandler() : oldHandlers_(numIds) {}
    ~RequestHandler() {
        cleanup();
    }


    /**
     * Returns the request ID from the message. This is not always the same field, so the actual handler class must provide it.
     * This uses the CRTP pattern to call the derived class's requestId method.
     *
     * @param msg The message to get the request ID from.
     * @returns The request ID from the message.
     */
    unsigned long requestId(const SIMCONNECT_RECV* msg) const {
        return static_cast<const T*>(this)->requestId(msg);
    }


    /**
     * Enable the responsive handler by registering it with the given message type ID. The current handler will be called if
     * we don't know the request associated with this message.
     * 
     * @tparam ConnectionType The connection type.
     * @tparam HandlerType The handler type.
     * @param handler The handler to hook into.
     */
    template <class ConnectionType, class HandlerType>
    void enable(Handler<ConnectionType, HandlerType>& msgHandler) {
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
     * Registers a handler for the given request ID.
     * 
     * @param requestId The request ID.
     * @param requestHandler The handler to register.
     * @param autoRemove True to automatically remove the handler after it has been called.
     */
    void registerHandler(unsigned long requestId, HandlerProc requestHandler, bool autoRemove) {
        requestHandlers_[requestId] = std::make_tuple(requestHandler, autoRemove);
    }


    /**
     * Remove a registration for the given request ID.
     * 
     * @note If the handler has already been removed, this will do nothing.
     * 
     * @param requestId The request ID.
     */
    void removeHandler(unsigned long requestId) {
        requestHandlers_.erase(requestId);
    }

};

} // namespace SimConnect