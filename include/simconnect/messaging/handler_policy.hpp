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
 #include <optional>
 #include <vector>

 #include <simconnect/simconnect.hpp>


namespace SimConnect {

/**
 * CRTP base class for handler policies.
 * 
 * @tparam Derived The derived policy class (CRTP pattern)
 * @tparam M The message type to handle
 */
template <class Derived, class M = Messages::MsgBase>
class HandlerPolicy {
public:
    using message_type = M;

protected:
    HandlerPolicy() = default;
    HandlerPolicy(const HandlerPolicy&) = default;
    HandlerPolicy(HandlerPolicy&&) = default;
    HandlerPolicy& operator=(const HandlerPolicy&) = default;
    HandlerPolicy& operator=(HandlerPolicy&&) = default;
    ~HandlerPolicy() = default;

public:
    /**
     * Calls all handlers with the given message (CRTP dispatch).
     */
    void operator()(const message_type& msg) const {
        static_cast<const Derived*>(this)->operator()(msg);
    }

    /**
     * Returns true if there are any handlers registered (CRTP dispatch).
     */
    [[nodiscard]] bool hasHandlers() const noexcept {
        return static_cast<const Derived*>(this)->hasHandlers();
    }

    /**
     * Clears all handlers (CRTP dispatch).
     */
    void clear() {
        static_cast<Derived*>(this)->clear();
    }

    /**
     * Returns the default handler procedure (CRTP dispatch).
     */
    auto proc() const {
        return static_cast<const Derived*>(this)->proc();
    }

    /**
     * Returns the number of handlers (CRTP dispatch).
     */
    [[nodiscard]] std::size_t handlerCount() const noexcept {
        return static_cast<const Derived*>(this)->handlerCount();
    }

    /**
     * Sets a handler procedure (CRTP dispatch).
     */
    auto setProc(auto proc) {
        return static_cast<Derived*>(this)->setProc(std::move(proc));
    }
};

/**
 * The SingleHandlerPolicy class supports a single handler procedure for SimConnect messages.
 * 
 * @tparam M The type of the message to handle.
 */
template <class M = Messages::MsgBase>
class SingleHandlerPolicy : public HandlerPolicy<SingleHandlerPolicy<M>, M> {
public:
    using base_type = HandlerPolicy<SingleHandlerPolicy<M>, M>;
    using message_type = typename base_type::message_type;
    using handler_id_type = bool; // Dummy type, as we only support a single handler.
    using handler_proc_type = std::function<void(const message_type&)>;


private:
    std::optional<handler_proc_type> handler_;

public:
    SingleHandlerPolicy() = default;
    SingleHandlerPolicy(const SingleHandlerPolicy&) = default;
    SingleHandlerPolicy(SingleHandlerPolicy&&) = default;
    SingleHandlerPolicy& operator=(const SingleHandlerPolicy&) = default;
    SingleHandlerPolicy& operator=(SingleHandlerPolicy&&) = default;

    ~SingleHandlerPolicy() = default;


    /**
     * Clears the handler associated with the given id.
     */
    void clear(handler_id_type) {
        handler_.reset();
    }


    /**
     * Clears all handlers.
     */
    void clear() {
        handler_.reset();
    }


    /**
     * Returns the handler procedure associated with the given id.
     */
    handler_proc_type proc(handler_id_type) const {
        return handler_.value_or(handler_proc_type{});
    }


    /**
     * Returns the default handler procedure.
     */
    handler_proc_type proc() const {
        return handler_.value_or(handler_proc_type{});
    }

    
    /**
     * Calls all handlers with the given message.
     */
    void operator()(const message_type& msg) const {
        if (handler_) {
            (*handler_)(msg);
        }
    }

    
    /**
     * Returns true if there are any handlers registered.
     * 
     * @return true if there are any handlers registered, false otherwise.
     */
    [[nodiscard]] bool hasHandlers() const noexcept { 
        return handler_.has_value(); 
    }

    /**
     * Sets the handler procedure (for compatibility with MultiHandlerPolicy).
     * 
     * @param proc The handler procedure to set.
     * @return A dummy handler ID (always true).
     */
    handler_id_type setProc(handler_proc_type proc) {
        handler_ = std::move(proc);
        return true;
    }

    /**
     * Returns the number of handlers (always 0 or 1 for SingleHandlerPolicy).
     */
    [[nodiscard]] std::size_t handlerCount() const noexcept {
        return handler_.has_value() ? 1 : 0;
    }
};

template <class M = Messages::MsgBase>
class MultiHandlerPolicy : public HandlerPolicy<MultiHandlerPolicy<M>, M> {
public:
    using base_type = HandlerPolicy<MultiHandlerPolicy<M>, M>;
    using message_type = typename base_type::message_type;
    using handler_id_type = std::uint32_t;
    using handler_proc_type = std::function<void(const message_type&)>;

private:
    std::vector<std::pair<handler_id_type, handler_proc_type>> handlers_;
    handler_id_type nextId_ = 0;

public:
    MultiHandlerPolicy() = default;
    MultiHandlerPolicy(const MultiHandlerPolicy&) = default;
    MultiHandlerPolicy(MultiHandlerPolicy&&) = default;
    MultiHandlerPolicy& operator=(const MultiHandlerPolicy&) = default;
    MultiHandlerPolicy& operator=(MultiHandlerPolicy&&) = default;

    ~MultiHandlerPolicy() = default;


    /**
     * Adds a new handler procedure and returns its unique identifier.
     * 
     * @param proc The handler procedure to add.
     * @return The unique identifier of the added handler procedure.
     */
    handler_id_type setProc(handler_proc_type proc) {
        const auto id = nextId_++;
        handlers_.emplace_back(id, std::move(proc));
        return id;
    }

    
    /**
     * Clears the handler associated with the given id.
     * 
     * @param id The unique identifier of the handler procedure to clear.
     */
    void clear(handler_id_type id) {
        handlers_.erase(
            std::remove_if(handlers_.begin(), handlers_.end(),
                [id](const auto& pair) { return pair.first == id; }),
            handlers_.end());
    }

    
    /**
     * Clears all handlers.
     */
    void clear() {
        handlers_.clear();
    }

    
    /**
     * Returns the handler procedure associated with the given id.
     * 
     * @param id The unique identifier of the handler procedure to retrieve.
     * @return The handler procedure associated with the given id, or an empty function if not found.
     */
    handler_proc_type proc(handler_id_type id) const {
        const auto it = std::find_if(handlers_.begin(), handlers_.end(),
            [id](const auto& pair) { return pair.first == id; });
        return (it != handlers_.end()) ? it->second : handler_proc_type{};
    }
    

    /**
     * Returns the first handler procedure for compatibility.
     * 
     * @return The first handler procedure, or an empty function if no handlers are registered.
     */
    handler_proc_type proc() const {
        // Return first handler for compatibility
        return handlers_.empty() ? handler_proc_type{} : handlers_.front().second;
    }

    
    /**
     * Calls all handlers with the given message.
     * 
     * @param msg The message to pass to the handlers.
     */
    void operator()(const message_type& msg) const {
        for (const auto& [id, handler] : handlers_) {
            if (handler) {
                handler(msg);
            }
        }
    }
    
    [[nodiscard]] bool hasHandlers() const noexcept { 
        return !handlers_.empty(); 
    }
    
    [[nodiscard]] std::size_t handlerCount() const noexcept { 
        return handlers_.size(); 
    }
};

} // namespace SimConnect