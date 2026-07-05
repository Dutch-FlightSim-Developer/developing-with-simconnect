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

#include <string_view>

#include <simconnect/simconnect.hpp>
#include <simconnect/events/flow_event_handlers.hpp>
#include <simconnect/messaging/handler_policy.hpp>
#include <simconnect/messaging/registration.hpp>


namespace SimConnect {


#if MSFS_2024_SDK

/**
 * Facade for subscribing to SimConnect flow events.
 *
 * SimConnect delivers all flow events through a single subscription (there is no per-event-type
 * subscription). This class registers one dispatch proc with the underlying message handler on
 * the first subscriber, and removes it again once the last subscriber is gone, ref-counted via
 * the internal handler list.
 *
 * @tparam M The SimConnect message handler type (e.g. WindowsEventHandler<...>).
 */
template <class M>
class FlowEvents {
public:
    using handler_id_type = typename MultiHandlerPolicy<Messages::FlowEventMsg>::handler_id_type;
    using registration_type = Registration<handler_id_type>;


private:
    M& handler_;
    MultiHandlerPolicy<Messages::FlowEventMsg> handlers_;
    typename M::handler_id_type registrationId_{};


public:
    explicit FlowEvents(M& handler) : handler_(handler) {}

    FlowEvents(const FlowEvents&) = delete;
    FlowEvents(FlowEvents&&) = delete;
    FlowEvents& operator=(const FlowEvents&) = delete;
    FlowEvents& operator=(FlowEvents&&) = delete;

    ~FlowEvents() {
        if (handlers_.hasHandlers()) {
            handler_.unRegisterHandler(Messages::flowEvent, registrationId_);
            handler_.connection().unsubscribeFromFlowEvents();
        }
    }


    /**
     * Subscribe to flow events using a single handler.
     *
     * @param handler Single callback for all flow events. The event type is determined by the first argument passed to the callback.
     * @returns A registration that unsubscribes this handler (and, if it was the last one, from SimConnect itself) when stopped or destroyed.
     */
    [[nodiscard]]
    registration_type subscribe(std::function<void(FlowEventId, std::string_view)> handler) {
        return subscribeProc([h = std::move(handler)](const Messages::FlowEventMsg& msg) {
            h(static_cast<FlowEventId>(msg.FlowEvent), msg.FltPath);
        });
    }


    /**
     * Subscribe to all flow events, routing each to the matching handler in @p handlers.
     *
     * @param handlers Struct of per-event-type callbacks. Unset fields are skipped.
     * @returns A registration that unsubscribes this handler (and, if it was the last one, from SimConnect itself) when stopped or destroyed.
     */
    [[nodiscard]]
    registration_type subscribe(FlowEventHandlers handlers) {
        return subscribeProc([h = std::move(handlers)](const Messages::FlowEventMsg& msg) {
            dispatch(h, msg);
        });
    }


private:

    /**
     * Adds @p proc to the internal handler list, subscribing to SimConnect's flow events first if this is the first handler.
     */
    registration_type subscribeProc(std::function<void(const Messages::FlowEventMsg&)> proc) {
        if (!handlers_.hasHandlers()) {
            registrationId_ = handler_.template registerHandler<Messages::FlowEventMsg>(
                Messages::flowEvent,
                [this](const Messages::FlowEventMsg& msg) { handlers_(msg); });
            handler_.connection().subscribeToFlowEvents();
        }
        const auto id = handlers_.setProc(std::move(proc));
        return registration_type(id, [this, id]() { unsubscribe(id); });
    }


    /**
     * Removes the handler with the given id, unsubscribing from SimConnect's flow events if it was the last one.
     */
    void unsubscribe(handler_id_type id) {
        handlers_.clear(id);
        if (!handlers_.hasHandlers()) {
            handler_.unRegisterHandler(Messages::flowEvent, registrationId_);
            handler_.connection().unsubscribeFromFlowEvents();
        }
    }


    static void dispatch(const FlowEventHandlers& h, const Messages::FlowEventMsg& msg) {
        switch (static_cast<FlowEventId>(msg.FlowEvent)) {
        case FlowEventIds::fltLoad:
            if (h.fltLoad)        { h.fltLoad(msg.FltPath); }        break;
        case FlowEventIds::fltLoaded:
            if (h.fltLoaded)      { h.fltLoaded(msg.FltPath); }      break;
        case FlowEventIds::teleportStart:
            if (h.teleportStart)  { h.teleportStart(); }              break;
        case FlowEventIds::teleportDone:
            if (h.teleportDone)   { h.teleportDone(); }               break;
        case FlowEventIds::backOnTrackStart:
            if (h.backOnTrackStart) { h.backOnTrackStart(); }         break;
        case FlowEventIds::backOnTrackDone:
            if (h.backOnTrackDone)  { h.backOnTrackDone(); }          break;
        case FlowEventIds::skipStart:
            if (h.skipStart)      { h.skipStart(); }                  break;
        case FlowEventIds::skipDone:
            if (h.skipDone)       { h.skipDone(); }                   break;
        case FlowEventIds::backToMainMenu:
            if (h.backToMainMenu) { h.backToMainMenu(); }             break;
        case FlowEventIds::rtcStart:
            if (h.rtcStart)       { h.rtcStart(); }                   break;
        case FlowEventIds::rtcEnd:
            if (h.rtcEnd)         { h.rtcEnd(); }                     break;
        case FlowEventIds::replayStart:
            if (h.replayStart)    { h.replayStart(); }                break;
        case FlowEventIds::replayEnd:
            if (h.replayEnd)      { h.replayEnd(); }                  break;
        case FlowEventIds::flightStart:
            if (h.flightStart)    { h.flightStart(); }                break;
        case FlowEventIds::flightEnd:
            if (h.flightEnd)      { h.flightEnd(); }                  break;
        case FlowEventIds::planeCrash:
            if (h.planeCrash)     { h.planeCrash(); }                 break;
        default:
            break;
        }
    }
};

#else
#error "flow_events.hpp requires the MSFS 2024 SDK (Flow Events are not available in earlier SDK versions)."
#endif // MSFS_2024_SDK


} // namespace SimConnect
