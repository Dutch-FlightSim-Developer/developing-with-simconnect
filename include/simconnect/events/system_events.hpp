#pragma once
/*
 * Copyright (c) 2024-2026. Bert Laverman
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
#include <vector>

#include <simconnect/simconnect.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/events.hpp>


namespace SimConnect {



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
    std::vector<typename EventHandler<M>::registration_type> registrations_;


public:
    SystemEvents(EventHandler<M>& handler) : handler_(handler)
    {
    }
    SystemEvents(const SystemEvents&) = delete;
    SystemEvents(SystemEvents&&) = default;
    SystemEvents& operator=(const SystemEvents&) = delete;
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


#pragma region System Events

    /**
     * Subscribe to a system event, receiving the raw EventMsg.
     * 
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the raw EventMsg.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(const Messages::EventMsg&)> handler) {
        registrations_.push_back(handler_.template registerEventHandler<Messages::EventMsg>(systemStateEvent, std::move(handler), false));
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Subscribe to a system event with a handler that receives the specific event message type.
     *
     * @tparam EventType The specific event message type (Messages::EventMsg, Messages::EventFilenameMsg, etc.)
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the specific event message type.
     */
    template <typename EventType>
    void subscribeToSystemEvent(event systemStateEvent,
                               std::function<void(const EventType&)> handler) {
        registrations_.push_back(handler_.template registerEventHandler<EventType>(systemStateEvent, std::move(handler), false));
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Subscribe to a filename-bearing system event (FlightLoaded, AircraftLoaded, etc.).
     * The handler receives the filename/path as a std::string_view.
     * 
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the filename/path as a std::string_view.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent<Messages::EventFilenameMsg>(systemStateEvent,
            [h = std::move(handler)](const Messages::EventFilenameMsg& msg) {
                h(msg.szFileName);
            });
    }

    /**
     * Subscribe to an object add/remove system event (ObjectAdded, ObjectRemoved).
     * The handler receives the SimObjectId and SimObjectType of the affected object.
     *
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the SimObjectId and SimObjectType of the affected object.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(SimObjectId, SimObjectType)> handler) {
        subscribeToSystemEvent<Messages::EventObjectAddRemoveMsg>(systemStateEvent,
            [h = std::move(handler)](const Messages::EventObjectAddRemoveMsg& msg) {
                h(static_cast<SimObjectId>(msg.dwData), msg.eObjType);
            });
    }

    /**
     * Subscribe to a frame-rate system event (Frame, PauseFrame).
     * The handler receives the frame rate (fps) and simulation speed as floats.
     *
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the frame rate (fps) and simulation speed as floats.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(float, float)> handler) {
        subscribeToSystemEvent<Messages::EventFrameMsg>(systemStateEvent,
            [h = std::move(handler)](const Messages::EventFrameMsg& msg) {
                h(msg.fFrameRate, msg.fSimSpeed);
            });
    }

    /**
     * Subscribe to a value-bearing system event (Sim, Pause, Sound, View, etc.).
     * The handler receives the event data value.
     *
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received. This receives the event data value.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent<Messages::EventMsg>(systemStateEvent,
            [h = std::move(handler)](const Messages::EventMsg& msg) {
                h(msg.dwData);
            });
    }

    /**
     * Subscribe to a simple system event with no meaningful data (SimStart, SimStop, Crashed, etc.).
     * 
     * @param systemStateEvent The event to subscribe to.
     * @param handler The handler to call when the event is received.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void()> handler) {
        subscribeToSystemEvent<Messages::EventMsg>(systemStateEvent,
            [h = std::move(handler)]([[maybe_unused]] const Messages::EventMsg& msg) {
                h();
            });
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
     * Enable a specific (already subscribed) system event.
     * 
     * @param systemStateEvent The event to enable.
     */
    void enableSystemEvent(event systemStateEvent) {
        handler_.connection().setSystemEventState(systemStateEvent, true);
    }


    /**
     * Disable a specific (already subscribed) system event.
     * 
     * @param systemStateEvent The event to disable.
     */
    void disableSystemEvent(event systemStateEvent) {
        handler_.connection().setSystemEventState(systemStateEvent, false);
    }

#pragma endregion // System Events

#pragma region Event-specific subscription methods

    // --- Filename events ---

    void onAircraftLoaded(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(handler_.connection().event("AircraftLoaded"), std::move(handler));
    }
    void onFlightLoaded(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(handler_.connection().event("FlightLoaded"), std::move(handler));
    }
    void onFlightSaved(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(handler_.connection().event("FlightSaved"), std::move(handler));
    }
    void onFlightPlanActivated(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(handler_.connection().event("FlightPlanActivated"), std::move(handler));
    }

    // --- Object add/remove events ---

    void onObjectAdded(std::function<void(SimObjectId, SimObjectType)> handler) {
        subscribeToSystemEvent(handler_.connection().event("ObjectAdded"), std::move(handler));
    }
    void onObjectRemoved(std::function<void(SimObjectId, SimObjectType)> handler) {
        subscribeToSystemEvent(handler_.connection().event("ObjectRemoved"), std::move(handler));
    }

    // --- Frame rate events ---

    void onFrame(std::function<void(float, float)> handler) {
        subscribeToSystemEvent(handler_.connection().event("Frame"), std::move(handler));
    }
    void onPauseFrame(std::function<void(float, float)> handler) {
        subscribeToSystemEvent(handler_.connection().event("PauseFrame"), std::move(handler));
    }

    // --- Value events ---

    void onSim(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("Sim"), std::move(handler));
    }
    void onPause(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("Pause"), std::move(handler));
    }
    void onPauseEx1(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("Pause_EX1"), std::move(handler));
    }
    void onSound(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("Sound"), std::move(handler));
    }
    void onView(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("View"), std::move(handler));
    }
    void onWeatherModeChanged(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(handler_.connection().event("WeatherModeChanged"), std::move(handler));
    }

    // --- Simple (no-data) events ---

    void onCrashed(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("Crashed"), std::move(handler));
    }
    void onCrashReset(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("CrashReset"), std::move(handler));
    }
    void onFlightPlanDeactivated(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("FlightPlanDeactivated"), std::move(handler));
    }
    void onOneSec(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("1sec"), std::move(handler));
    }
    void onFourSec(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("4sec"), std::move(handler));
    }
    void onSixHz(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("6Hz"), std::move(handler));
    }
    void onPaused(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("Paused"), std::move(handler));
    }
    void onUnpaused(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("Unpaused"), std::move(handler));
    }
    void onPositionChanged(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("PositionChanged"), std::move(handler));
    }
    void onSimStart(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("SimStart"), std::move(handler));
    }
    void onSimStop(std::function<void()> handler) {
        subscribeToSystemEvent(handler_.connection().event("SimStop"), std::move(handler));
    }

#pragma endregion

};

} // namespace SimConnect