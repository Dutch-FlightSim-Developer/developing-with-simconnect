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
}