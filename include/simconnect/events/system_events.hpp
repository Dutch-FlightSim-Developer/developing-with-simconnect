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

#include <simconnect/events/events.hpp>


namespace SimConnect {


    namespace Events {

        event oneSec() { return event::get("1sec"); };
        event fourSec() { return event::get("4sec"); };
        event sixHerz() { return event::get("6Hz"); };
        event aircraftLoaded() { return event::get("AircraftLoaded"); };
        event crashed() { return event::get("Crashed"); };
        event crashReset() { return event::get("CrashReset"); };
        event customMissionActionExecuted() { return event::get("CustomMissionActionExecuted"); };             // Legacy
        event flightLoaded() { return event::get("FlightLoaded"); };
        event flightSaved() { return event::get("FlightSaved"); };
        event flightPlanActivated() { return event::get("FlightPlanActivated"); };
        event flightPlanDeactivated() { return event::get("FlightPlanDeactivated"); };
        event frame() { return event::get("Frame"); };
        event objectAdded() { return event::get("ObjectAdded"); };
        event objectRemoved() { return event::get("ObjectRemoved"); };
        event pause() { return event::get("Pause"); };
        event pause_EX1() { return event::get("Pause_EX1"); };
        event paused() { return event::get("Paused"); };
        event pauseFrame() { return event::get("PauseFrame"); };
        event positionChanged() { return event::get("PositionChanged"); };
        event sim() { return event::get("Sim"); };
        event simStart() { return event::get("SimStart"); };
        event simStop() { return event::get("SimStop"); };
        event sound() { return event::get("Sound"); };
        event unpaused() { return event::get("Unpaused"); };
        event view() { return event::get("View"); };
        event weatherModeChanged() { return event::get("WeatherModeChanged"); };
    }
}