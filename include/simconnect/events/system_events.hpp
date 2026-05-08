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


#include <string_view>

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


#pragma region Core subscribeToSystemEvent overloads

    /**
     * Subscribe to a system event, receiving the raw EventMsg.
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(const Messages::EventMsg&)> handler) {
        handler_.template registerEventHandler<Messages::EventMsg>(systemStateEvent, std::move(handler), false);
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Subscribe to a system event with a handler that receives the specific event message type.
     *
     * @tparam EventType The specific event message type (Messages::EventMsg, Messages::EventFilenameMsg, etc.)
     */
    template <typename EventType>
    void subscribeToSystemEvent(event systemStateEvent,
                               std::function<void(const EventType&)> handler) {
        handler_.template registerEventHandler<EventType>(systemStateEvent, std::move(handler), false);
        handler_.connection().subscribeToSystemEvent(systemStateEvent);
    }

    /**
     * Subscribe to a filename-bearing system event (FlightLoaded, AircraftLoaded, etc.).
     * The handler receives the filename/path as a std::string_view.
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
     */
    void subscribeToSystemEvent(event systemStateEvent, std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent<Messages::EventMsg>(systemStateEvent,
            [h = std::move(handler)](const Messages::EventMsg& msg) {
                h(msg.dwData);
            });
    }

    /**
     * Subscribe to a simple system event with no meaningful data (SimStart, SimStop, Crashed, etc.).
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

#pragma endregion

#pragma region Event-specific subscription methods

    // --- Filename events ---

    void onAircraftLoaded(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(Events::aircraftLoaded(), std::move(handler));
    }
    void onFlightLoaded(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(Events::flightLoaded(), std::move(handler));
    }
    void onFlightSaved(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(Events::flightSaved(), std::move(handler));
    }
    void onFlightPlanActivated(std::function<void(std::string_view)> handler) {
        subscribeToSystemEvent(Events::flightPlanActivated(), std::move(handler));
    }

    // --- Object add/remove events ---

    void onObjectAdded(std::function<void(SimObjectId, SimObjectType)> handler) {
        subscribeToSystemEvent(Events::objectAdded(), std::move(handler));
    }
    void onObjectRemoved(std::function<void(SimObjectId, SimObjectType)> handler) {
        subscribeToSystemEvent(Events::objectRemoved(), std::move(handler));
    }

    // --- Frame rate events ---

    void onFrame(std::function<void(float, float)> handler) {
        subscribeToSystemEvent(Events::frame(), std::move(handler));
    }
    void onPauseFrame(std::function<void(float, float)> handler) {
        subscribeToSystemEvent(Events::pauseFrame(), std::move(handler));
    }

    // --- Value events ---

    void onSim(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::sim(), std::move(handler));
    }
    void onPause(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::pause(), std::move(handler));
    }
    void onPauseEx1(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::pause_EX1(), std::move(handler));
    }
    void onSound(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::sound(), std::move(handler));
    }
    void onView(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::view(), std::move(handler));
    }
    void onWeatherModeChanged(std::function<void(unsigned long)> handler) {
        subscribeToSystemEvent(Events::weatherModeChanged(), std::move(handler));
    }

    // --- Simple (no-data) events ---

    void onCrashed(std::function<void()> handler) {
        subscribeToSystemEvent(Events::crashed(), std::move(handler));
    }
    void onCrashReset(std::function<void()> handler) {
        subscribeToSystemEvent(Events::crashReset(), std::move(handler));
    }
    void onFlightPlanDeactivated(std::function<void()> handler) {
        subscribeToSystemEvent(Events::flightPlanDeactivated(), std::move(handler));
    }
    void onOneSec(std::function<void()> handler) {
        subscribeToSystemEvent(Events::oneSec(), std::move(handler));
    }
    void onFourSec(std::function<void()> handler) {
        subscribeToSystemEvent(Events::fourSec(), std::move(handler));
    }
    void onSixHz(std::function<void()> handler) {
        subscribeToSystemEvent(Events::sixHerz(), std::move(handler));
    }
    void onPaused(std::function<void()> handler) {
        subscribeToSystemEvent(Events::paused(), std::move(handler));
    }
    void onUnpaused(std::function<void()> handler) {
        subscribeToSystemEvent(Events::unpaused(), std::move(handler));
    }
    void onPositionChanged(std::function<void()> handler) {
        subscribeToSystemEvent(Events::positionChanged(), std::move(handler));
    }
    void onSimStart(std::function<void()> handler) {
        subscribeToSystemEvent(Events::simStart(), std::move(handler));
    }
    void onSimStop(std::function<void()> handler) {
        subscribeToSystemEvent(Events::simStop(), std::move(handler));
    }

#pragma endregion

};

} // namespace SimConnect