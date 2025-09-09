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


#include <variant>

#include <simconnect/messaging/handler_proc.hpp>


namespace SimConnect {


/**
 * The SimpleHandlerProc class is a specialization of HandlerProc for a single handler function.
 * 
 * @tparam message_type The type of the message to handle.
 */
template <typename message_type>
class SimpleHandlerProc : public HandlerProc<message_type, std::monostate, SimpleHandlerProc<message_type>> {

public:
    using HandlerProcType = typename HandlerProc<message_type, std::monostate, SimpleHandlerProc<message_type>>::HandlerProcType;

private:
    HandlerProcType proc_;

public:
    SimpleHandlerProc() = default;
    SimpleHandlerProc(const SimpleHandlerProc&) = default;
    SimpleHandlerProc(SimpleHandlerProc&&) = default;
    SimpleHandlerProc& operator=(const SimpleHandlerProc&) = default;
    SimpleHandlerProc& operator=(SimpleHandlerProc&&) = default;
    ~SimpleHandlerProc() = default;


    SimpleHandlerProc(HandlerProcType proc) {
        setProc(proc);
    }


    /**
     * Returns the handler function.
     */
    HandlerProcType proc([[maybe_unused]] std::monostate id) const {
        return proc_;
    }


    /**
     * Returns _the_ handler function.
     */
    HandlerProcType proc() const {
        return proc_;
    }


    /**
     * Sets the handler function.
     *
     * @param proc The handler function to set.
     * @returns Nothing
     */
    std::monostate setProc(HandlerProcType proc) {
        proc_ = proc;

        return {};
    }


    /**
     * Clears the handler function with the given id.
     * 
     * @param id The ID of the handler to clear.
     */
    void clear([[maybe_unused]] std::monostate id) {
        proc_ = nullptr;
    }


    /**
     * Clears the handler function.
     */
    void clear() {
        proc_ = nullptr;
    }


    /**
     * Calls the handler with the given message.
     * 
     * @param msg The message to handle.
     */
    void operator()(const message_type& msg) const {
        if (proc_) {
            proc_(msg);
        }
    }
};

} // namespace SimConnect