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


#include <simconnect/simconnect.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/events.hpp>


namespace SimConnect {


namespace Events {

    inline event oneSec() { return event::get("1sec"); };
    inline event fourSec() { return event::get("4sec"); };
    inline event sixHerz() { return event::get("6Hz"); };
    inline event aircraftLoaded() { return event::get("AircraftLoaded"); };
    inline event crashed() { return event::get("Crashed"); };
    inline event crashReset() { return event::get("CrashReset"); };
    inline event customMissionActionExecuted() { return event::get("CustomMissionActionExecuted"); };             // Legacy
    inline event flightLoaded() { return event::get("FlightLoaded"); };
    inline event flightSaved() { return event::get("FlightSaved"); };
    inline event flightPlanActivated() { return event::get("FlightPlanActivated"); };
    inline event flightPlanDeactivated() { return event::get("FlightPlanDeactivated"); };
    inline event frame() { return event::get("Frame"); };
    inline event objectAdded() { return event::get("ObjectAdded"); };
    inline event objectRemoved() { return event::get("ObjectRemoved"); };
    inline event pause() { return event::get("Pause"); };
    inline event pause_EX1() { return event::get("Pause_EX1"); };
    inline event paused() { return event::get("Paused"); };
    inline event pauseFrame() { return event::get("PauseFrame"); };
    inline event positionChanged() { return event::get("PositionChanged"); };
    inline event sim() { return event::get("Sim"); };
    inline event simStart() { return event::get("SimStart"); };
    inline event simStop() { return event::get("SimStop"); };
    inline event sound() { return event::get("Sound"); };
    inline event unpaused() { return event::get("Unpaused"); };
    inline event view() { return event::get("View"); };
    inline event weatherModeChanged() { return event::get("WeatherModeChanged"); };
}


/**
 * This class provides convenient subscription methods for system events.
 * 
 * This is a lightweight facade over EventHandler that handles system-specific
 * event subscriptions (sim start/stop, flight loaded, position changed, etc.).
 * 
 * Multiple SystemEvents instances can safely share the same EventHandler, as
 * event IDs are globally unique and managed by the underlying handler.
 * 
 * @tparam M The SimConnect message handler type
 */
template<class M>
class SystemEvents
{
    EventHandler<M>& handler_;


public:
    SystemEvents(EventHandler<M>& handler) : handler_(handler)
    {
    }
    SystemEvents(const SystemEvents&) = default;
    SystemEvents(SystemEvents&&) = default;
    SystemEvents& operator=(const SystemEvents&) = default;
    SystemEvents& operator=(SystemEvents&&) = default;
    ~SystemEvents() = default;


    /**
     * Get the underlying EventHandler.
     * 
     * @returns The underlying EventHandler.
     */
    EventHandler<M>& handler() {
        return handler_;
    }


    /**
     * Subscribe to a system event.
     * 
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(const Messages::EventMsg&)> handler) {
        handler_.template registerEventHandler<Messages::EventMsg>(systemStateEvent, std::move(handler), false);
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Unsubscribe from a system event.
     * 
     * @param systemStateEvent The event to unsubscribe from.
     */
    void unsubscribeFromSystemEvent(event systemStateEvent) {
        handler_.connection().unsubscribeFromSystemEvent(systemStateEvent);
        handler_.removeHandler(systemStateEvent);
    }


    /**
     * Subscribe to a system event with a handler that receives the specific event message type.
     * 
     * @tparam EventType The specific event message type (Messages::Event, etc.)
     * @param systemStateEvent The event to subscribe to.
     * @param handler The typed handler to call when the event is received.
     */
    template <typename EventType>
    void subscribeToSystemEvent(event systemStateEvent, 
                               std::function<void(const EventType&)> handler) {
        handler_.template registerEventHandler<EventType>(systemStateEvent, std::move(handler), false);
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }
};

} // namespace SimConnect