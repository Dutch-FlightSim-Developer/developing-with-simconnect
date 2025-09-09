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


#include <vector>

#include <simconnect/messaging/handler_proc.hpp>


namespace SimConnect {


/**
 * The MultiHandlerProc class is a specialization of HandlerProc for multiple handler functions.
 * 
 * @tparam message_type The type of the message to handle.
 */
template <typename message_type>
class MultiHandlerProc : public HandlerProc<message_type, size_t, MultiHandlerProc<message_type>> {

public:
    using HandlerProcType = typename HandlerProc<message_type, size_t, MultiHandlerProc<message_type>>::HandlerProcType;

private:
    std::vector<HandlerProcType> procs_;

public:
    MultiHandlerProc() = default;
    MultiHandlerProc(const MultiHandlerProc&) = default;
    MultiHandlerProc(MultiHandlerProc&&) = default;
    MultiHandlerProc& operator=(const MultiHandlerProc&) = default;
    MultiHandlerProc& operator=(MultiHandlerProc&&) = default;
    ~MultiHandlerProc() = default;


    MultiHandlerProc(HandlerProcType proc) {
        setProc(proc);
    }


    /**
     * Returns the handler function.
     */
    HandlerProcType proc(size_t id) const {
        return (id < procs_.size()) ? procs_[id] : nullptr;
    }


    /**
     * Returns _the_ handler function.
     */
    HandlerProcType proc() const {
        return nullptr; // MultiHandlerProc does not have a single proc
    }


    /**
     * Clears the handler function with the given id.
     * 
     * @param id The ID of the handler to clear.
     */
    void clear(size_t id) {
        if (id < procs_.size()) {
            procs_[id] = nullptr;
        }
    }


    /**
     * Clears all handler functions.
     */
    void clear() {
        procs_.clear();
    }


    /**
     * Sets the handler function.
     *
     * @param proc The handler function to set.
     * @returns The ID of the new handler.
     */
    size_t setProc(HandlerProcType proc) {
        size_t id = procs_.size();

        procs_.push_back(proc);

        return id; // Return the ID of the new handler
    }


    /**
     * Calls the handler with the given message.
     * 
     * @param msg The message to handle.
     */
    void operator()(const message_type& msg) const {
        for (const auto& proc : procs_) {
            if (proc) {
                proc(msg);
            }
        }
    }
};

} // namespace SimConnect