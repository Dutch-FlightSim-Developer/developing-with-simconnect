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
#include <simconnect/events/events.hpp>


namespace SimConnect {


using BaseSystemEventHandler = std::function<void(const SIMCONNECT_RECV_EVENT&)>;


/**
 * The EventHandler class provides for responsive handling of events.
 */
class EventHandler  {
    std::map<unsigned long, BaseSystemEventHandler> handlers_;
    std::function<void()> cleanup_;


    /**
     * Dispatches a message, if we have a handler for it.
     * 
     * @param msg The message to dispatch.
     * @returns true if we had a handler for the associated event ID.
     */
    [[nodiscard]]
    bool dispatch(const SIMCONNECT_RECV_EVENT& msg) {
        auto eventId = msg.uEventID;
        auto it = handlers_.find(eventId);
        if (it != handlers_.end()) {
            it->second(msg);

            return true;
        }
        return false;
    }


    /**
     * Registers a handler for the given event ID.
     * 
     * @param eventId The event ID.
     * @param handler The handler to register.
     */
    void registerHandler(unsigned long eventId, BaseSystemEventHandler handler) {
        handlers_[eventId] = handler;
    }


    /**
     * Remove a registration for the given event ID.
     * 
     * @param eventId The event ID.
     */
    void removeHandler(unsigned long eventId) {
        handlers_.erase(eventId);
    }


public:
    EventHandler() = default;
    ~EventHandler() {
        clear();
    }

    // No copies or moves
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;


    /**
     * Unsubscribes all currently registered events, resets the HandlerProc, and clears the handler list.
     * 
     * @see #enable()
     */
    void clear() {
        if (cleanup_) {
            cleanup_();
        }
        handlers_.clear();
    }


    /**
     * Enable the responsive handler by registering it with the given message type ID. The current handler will be called if
     * we don't know the event associated with this message.
     * 
     * @tparam ConnectionType The connection type.
     * @tparam HandlerType The handler type.
     * @param handler The handler to hook into.
     * @param id The message type ID.
     */
    template <class ConnectionType, class HandlerType>
    void enable(Handler<ConnectionType, HandlerType>& handler) {
        clear();    // Clear any existing handlers.

        auto originalHandlerProc = handler.getHandler(SIMCONNECT_RECV_ID_EVENT);
        auto defaultHandlerProc = handler.defaultHandler();

        handler.registerHandler<SIMCONNECT_RECV_EVENT>(SIMCONNECT_RECV_ID_EVENT,
            [this, originalHandlerProc, defaultHandlerProc](const SIMCONNECT_RECV_EVENT& msg)
            {
                if (!dispatch(msg)) {
                    if (originalHandlerProc) {
                        originalHandlerProc(&msg, sizeof(msg));
                    } else if (defaultHandlerProc) {
                        defaultHandlerProc(&msg, sizeof(msg));
                    }
                }
            });

        cleanup_ = [this, &handler, originalHandlerProc]() {
            for (auto& [eventId, _] : handlers_) {
                handler.connection().unsubscribeFromSystemEvent(event::get(eventId));
            }
            handler.registerHandlerProc(SIMCONNECT_RECV_ID_EVENT, originalHandlerProc); 
        };
    }


    /**
     * Subscribe to a system event.
     * 
     * @param connection The connection to subscribe with.
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received.
     */
    void subscribeToSystemEvent(Connection& connection, event systemStateEvent, BaseSystemEventHandler handler) {
        registerHandler(systemStateEvent, handler);
        connection.subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Unsubscribe from a system event.
     * 
     * @param connection The connection to unsubscribe from.
     * @param systemStateEvent The event to unsubscribe from.
     */
    void unsubscribeFromSystemEvent(Connection& connection, event systemStateEvent) {
        connection.unsubscribeFromSystemEvent(systemStateEvent);
        removeHandler(systemStateEvent);
    }
};

} // namespace SimConnect