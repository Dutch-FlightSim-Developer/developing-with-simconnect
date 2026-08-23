#pragma once
/*
 * Copyright (c) 2026. Bert Laverman
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
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>


namespace SimConnect {

template <class M>
class InputEventHandler : public MessageHandler<RequestId, InputEventHandler<M>, M, Messages::enumerateInputEvents, Messages::getInputEvent> {
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using handler_type = typename M::handler_type;

private:
    /**
     * SimConnect_EnumerateInputEventParams, unlike the other input-event calls, has no RequestID
     * of its own - only the Hash to correlate a response back to its request. That means it needs
     * its own MessageHandler instance, keyed by InputEventHash rather than RequestId. Kept private:
     * callers only ever see InputEventHandler::enumerateInputEventParams().
     */
    class ParamsHandler : public MessageHandler<InputEventHash, ParamsHandler, M, Messages::enumerateInputEventParams> {
    public:
        using simconnect_message_handler_type = M;

    private:
        simconnect_message_handler_type& simConnectMessageHandler_;

        ParamsHandler(const ParamsHandler&) = delete;
        ParamsHandler(ParamsHandler&&) = delete;
        ParamsHandler& operator=(const ParamsHandler&) = delete;
        ParamsHandler& operator=(ParamsHandler&&) = delete;

    public:
        explicit ParamsHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
        {
            this->enable(simConnectMessageHandler_);
        }
        ~ParamsHandler() = default;

        InputEventHash correlationId(const Messages::MsgBase& msg) {
            return reinterpret_cast<const Messages::EnumerateInputEventParamsMsg&>(msg).Hash; //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }

        void enumerateInputEventParams(InputEventHash hash, std::function<void(std::string_view)> handler) {
            this->registerHandler(hash, [handler](const Messages::MsgBase& msg) {
                const auto& paramsMsg = reinterpret_cast<const Messages::EnumerateInputEventParamsMsg&>(msg); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                handler(std::string_view(&paramsMsg.Value[0]));
            }, true);
            simConnectMessageHandler_.connection().enumerateInputEventParams(hash);
        }
    };

    simconnect_message_handler_type& simConnectMessageHandler_;
    ParamsHandler paramsHandler_;

    // No copies or moves
    InputEventHandler(const InputEventHandler&) = delete;
    InputEventHandler(InputEventHandler&&) = delete;
    InputEventHandler& operator=(const InputEventHandler&) = delete;
    InputEventHandler& operator=(InputEventHandler&&) = delete;

public:
    explicit InputEventHandler(simconnect_message_handler_type& handler)
        : simConnectMessageHandler_(handler)
        , paramsHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~InputEventHandler() = default;


    /**
     * Returns the request ID from the message. This is specific to the Messages::EnumerateInputEventsMsg
     * and Messages::GetInputEventMsg messages.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    RequestId correlationId(const Messages::MsgBase& msg) {
        switch (msg.dwID) {
        case Messages::enumerateInputEvents:
            return reinterpret_cast<const Messages::EnumerateInputEventsMsg&>(msg).dwRequestID; //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        case Messages::getInputEvent:
            return reinterpret_cast<const Messages::GetInputEventMsg&>(msg).dwRequestID; //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        default:
            this->logger().warn("Received unknown message ID {} when trying to get correlation ID.", static_cast<int>(msg.dwID));
            break;
        }
        return noRequest;
    }


    /**
     * Requests the enumeration of input events declared by the loaded aircraft, invoking the
     * provided handler for each one.
     *
     * @param handler The handler to invoke for each declared input event.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request enumerateInputEvents(std::function<void(const InputEvent&)> handler, std::function<void()> onDone = nullptr) {
        return enumerateInputEvents([handler](InputEventHash hash, std::string_view name, InputEventType type) {
            handler(InputEvent{ .name = std::string(name), .hash = hash, .type = type });
        }, std::move(onDone));
    }


    /**
     * Requests the enumeration of input events declared by the loaded aircraft, invoking the
     * provided handler for each one with its fields as separate parameters. Avoids constructing
     * an InputEvent (and copying its name into a std::string) per entry; prefer this over the
     * InputEvent-taking overload when enumerating large lists on a hot path.
     *
     * @param handler The handler to invoke for each declared input event.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request enumerateInputEvents(std::function<void(InputEventHash hash, std::string_view name, InputEventType type)> handler, std::function<void()> onDone = nullptr) {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler, onDone](const Messages::MsgBase& msg) {
            const auto& eventsMsg = reinterpret_cast<const Messages::EnumerateInputEventsMsg&>(msg); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

            for (unsigned long i = 0; i < eventsMsg.dwArraySize; ++i) {
                const auto& item = eventsMsg.rgData[i];
                handler(item.Hash, std::string_view(&item.Name[0]), static_cast<InputEventType>(item.eType));
            }

            if (eventsMsg.dwEntryNumber == (eventsMsg.dwOutOf - 1)) { // 0 to dwOutOf-1
                if (onDone) {
                    onDone();
                }
            }
        }, false);
        simConnectMessageHandler_.connection().enumerateInputEvents(requestId);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }


    /**
     * Requests the current value of a double-valued input event.
     *
     * @param hash The hash of the input event.
     * @param handler The handler to invoke with the value.
     */
    void getInputEvent(InputEventHash hash, std::function<void(double)> handler) {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            const auto& eventMsg = reinterpret_cast<const Messages::GetInputEventMsg&>(msg); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            handler(*reinterpret_cast<const double*>(&eventMsg.Value)); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }, true);
        simConnectMessageHandler_.connection().getInputEvent(requestId, hash);
    }


    /**
     * Requests the current value of a string-valued input event.
     *
     * @param hash The hash of the input event.
     * @param handler The handler to invoke with the value.
     */
    void getInputEvent(InputEventHash hash, std::function<void(std::string)> handler) {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            const auto& eventMsg = reinterpret_cast<const Messages::GetInputEventMsg&>(msg); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            handler(std::string(reinterpret_cast<const char*>(&eventMsg.Value))); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }, true);
        simConnectMessageHandler_.connection().getInputEvent(requestId, hash);
    }


    /**
     * Requests the list of parameters an input event expects, as a raw ';'-separated type string
     * (see SimConnect_EnumerateInputEventParams).
     *
     * @param hash The hash of the input event.
     * @param handler The handler to invoke with the raw parameter type string.
     */
    void enumerateInputEventParams(InputEventHash hash, std::function<void(std::string_view)> handler) {
        paramsHandler_.enumerateInputEventParams(hash, std::move(handler));
    }


    /**
     * Finds a single declared input event by exact name. This is the common case for a real
     * add-on wiring up one known event, as opposed to enumerateInputEvents()'s per-entry
     * scan (better suited to diagnostics/tooling that don't know the exact name up front).
     *
     * @param name The exact (case-sensitive) name of the input event to find.
     * @param onFound Called with the matching event, if one is declared.
     * @param onNotFound Called once enumeration completes with no match found.
     * @return A Request object that can be used to stop the underlying enumeration.
     */
    [[nodiscard]]
    Request findInputEvent(std::string_view name, std::function<void(const InputEvent&)> onFound, std::function<void()> onNotFound = nullptr) {
        auto found = std::make_shared<bool>(false);
        return enumerateInputEvents(
            [name = std::string(name), found, onFound](InputEventHash hash, std::string_view candidateName, InputEventType type) {
                if (!*found && candidateName == name) {
                    *found = true;
                    onFound(InputEvent{ .name = std::string(candidateName), .hash = hash, .type = type });
                }
            },
            [found, onNotFound]() {
                if (!*found && onNotFound) {
                    onNotFound();
                }
            });
    }


    /**
     * Finds a single declared input event by exact name, then fetches its parameters - the
     * common case of "look it up and tell me how to call it" in one step.
     *
     * @param name The exact (case-sensitive) name of the input event to find.
     * @param onFound Called with the matching event and its raw ';'-separated parameter type
     *                string, if one is declared.
     * @param onNotFound Called once enumeration completes with no match found.
     * @return A Request object that can be used to stop the underlying enumeration. Note this
     *         does not cover the parameter fetch triggered internally once a match is found.
     */
    [[nodiscard]]
    Request findInputEvent(std::string_view name, std::function<void(const InputEvent&, std::string_view)> onFound, std::function<void()> onNotFound = nullptr) {
        return findInputEvent(name,
            [this, onFound](const InputEvent& event) {
                enumerateInputEventParams(event.hash, [event, onFound](std::string_view params) {
                    onFound(event, params);
                });
            },
            std::move(onNotFound));
    }

};

} // namespace SimConnect
