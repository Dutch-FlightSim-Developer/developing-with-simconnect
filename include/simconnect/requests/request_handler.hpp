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
#include <functional>


#include <simconnect/handler.hpp>


namespace SimConnect {

/**
 * The RequestHandler class provides for responsive handling of requests.
 */
class RequestHandler  {
#if defined(SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST)     // MSFS 2024
    /** Array of message handlers. */
    std::array<std::function<unsigned long(const SIMCONNECT_RECV*)>, SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST + 1> requestFinders_;
#else                                                                   // FSX
    /** Array of message handlers. */
    std::array<std::function<unsigned long(const SIMCONNECT_RECV*)>, SIMCONNECT_RECV_ID_EVENT_RACE_LAP + 1> requestFinders_;
#endif
    std::map<unsigned long, std::tuple<HandlerProc, bool>> handlers_;
    std::function<void()> cleanup_;


    /**
     * Add a function that can extract the requestId from a message.
     * 
     * @param finder The function to find the requestId.
     */
    void addRequestFinder(SIMCONNECT_RECV_ID id, std::function<unsigned long(const SIMCONNECT_RECV*)> finder) {
        requestFinders_[id] = finder;
    }


    /**
     * Add a finder for System State messages.
     */
    void addSystemStateRequestFinder() {
        if (!requestFinders_[SIMCONNECT_RECV_ID_SYSTEM_STATE]) {
            requestFinders_[SIMCONNECT_RECV_ID_SYSTEM_STATE] = [](const SIMCONNECT_RECV* msg) {
                return static_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg)->dwRequestID;
            };
        }
    }


    /**
     * Dispatches a message, if we have a handler for it.
     * 
     * @param msg The message to dispatch.
     * @param size The size of the message.
     * @returns true if we had a handler for the associated request ID.
     */
    [[nodiscard]]
    bool dispatch(const SIMCONNECT_RECV* msg, DWORD size) {
        if (requestFinders_[msg->dwID]) {
            auto requestId = requestFinders_[msg->dwID](msg);
            auto it = handlers_.find(requestId);
            if (it != handlers_.end()) {
                auto& [handler, remove] = it->second;
                handler(msg, size);
                if (remove) {
                    handlers_.erase(it);
                }
                return true;
            }
        }
        return false;
    }


public:
    RequestHandler() = default;
    ~RequestHandler() {
        if (cleanup_) {
            cleanup_();
        }
    }

    // No copies or moves
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler(RequestHandler&&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    RequestHandler& operator=(RequestHandler&&) = delete;


    /**
     * Enable the responsive handler by registering it with the given message type ID. The current handler will be called if
     * we don't know the request associated with this message.
     * 
     * @tparam ConnectionType The connection type.
     * @tparam HandlerType The handler type.
     * @param handler The handler to hook into.
     * @param id The message type ID.
     */
    template <class ConnectionType, class HandlerType>
    void enable(Handler<ConnectionType, HandlerType>& handler, SIMCONNECT_RECV_ID id) {
        if (cleanup_) { // Were we already linked somehow, unlink
            cleanup_();
            handlers_.clear();
        }
        auto originalHandlerProc = handler.getHandler(id);
        auto defaultHandlerProc = handler.defaultHandler();

        handler.registerHandlerProc(id, [this, id, originalHandlerProc, defaultHandlerProc](const SIMCONNECT_RECV* msg, DWORD size) {
            if (!dispatch(msg, size)) {
                if (originalHandlerProc) {
                    originalHandlerProc(msg, size);
                } else if (defaultHandlerProc) {
                    defaultHandlerProc(msg, size);
                }
            }
        });
        cleanup_ = [&handler, id, originalHandlerProc]() { handler.registerHandlerProc(id, originalHandlerProc); };
    }

    /**
     * Registers a handler for the given request ID.
     * 
     * @param requestId The request ID.
     * @param handler The handler to register.
     * @param autoRemove True to automatically remove the handler after it has been called.
     */
    void registerHandler(unsigned long requestId, HandlerProc handler, bool autoRemove) {
        handlers_[requestId] = std::make_tuple(handler, autoRemove);
    }


    /**
     * Remove a registration for the given request ID.
     * 
     * @param requestId The request ID.
     */
    void removeHandler(unsigned long requestId) {
        handlers_.erase(requestId);
    }


    /**
     * Requests a bool-valued system state.
     * 
     * @param connection The connection to request the state from.
     * @param name The name of the state to request.
     * @param handler The handler to execute once the state is received.
     */
    void requestSystemState(Connection& connection, std::string name, std::function<void(bool)> handler) {
        addSystemStateRequestFinder();

        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [handler](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD size) {
            auto& state = *reinterpret_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg);
            handler(state.dwInteger != 0);
        }, true);
        connection.requestSystemState(name, requestId);
    }


    /**
     * Requests a string-valued system state.
     * 
     * @param connection The connection to request the state from.
     * @param name The name of the state to request.
     * @param handler The handler to execute once the state is received.
     */
    void requestSystemState(Connection& connection, std::string name, std::function<void(std::string)> handler) {
        addSystemStateRequestFinder();

        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [handler](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD size) {
            auto& state = *reinterpret_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg);
            handler(std::string(state.szString));
        }, true);
        connection.requestSystemState(name, requestId);
    }
};

} // namespace SimConnect