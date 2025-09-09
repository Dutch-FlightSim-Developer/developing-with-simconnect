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


 #include <functional>

 #include <simconnect.hpp>
 #include <simconnect/util/crtp.hpp>


namespace SimConnect {


/**
 * The HandlerProc class is a base class for handler procedures that can be used with SimConnect messages.
 * 
 * @tparam message_type The type of the message to handle.
 * @tparam handler_id_type The type of the handler ID, which is used to identify the handler.
 * @tparam handler_type The type of the handler, which must derive from this class.
 */
template <typename message_type, typename handler_id_type, class handler_type>
class HandlerProc {
public:
    using MessageType = message_type;
    using HandlerProcType = std::function<void(const message_type&)>;


    HandlerProc() = default;
    HandlerProc(const HandlerProc&) = default;
    HandlerProc(HandlerProc&&) = default;
    HandlerProc& operator=(const HandlerProc&) = default;
    HandlerProc& operator=(HandlerProc&&) = default;


    /**
     * Returns the handler function.
     */
    HandlerProcType proc(handler_id_type id) const {
        return static_cast<const handler_type*>(this)->proc(id);
    }


    /**
     * Returns _the_ handler function.
     */
    HandlerProcType proc() const {
        return static_cast<const handler_type*>(this)->proc();
    }


    /**
     * Sets the handler function.
     * 
     * @param proc The handler function to set.
     * @returns The new handler's ID.
     */
    handler_id_type setProc(HandlerProcType proc) {
        return static_cast<handler_type*>(this)->setProc(proc);
    }


    /**
     * Clears the handler function with the given id.
     * 
     * @param id The ID of the handler to clear.
     */
    void clear(handler_id_type id) {
        static_cast<handler_type*>(this)->clear(id);
    }


    /**
     * Clears all handlers.
     */
    void clear() {
        static_cast<handler_type*>(this)->clear();
    }


    /**
     * Calls the handler with the given message.
     * 
     * @param msg The message to handle.
     */
    void operator()(const message_type& msg) const {
        static_cast<const handler_type*>(this)->operator()(msg);
    }
};


} // namespace SimConnect