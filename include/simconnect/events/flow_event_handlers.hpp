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
#include <string_view>


namespace SimConnect {


#if MSFS_2024_SDK

/**
 * Handlers for each flow event type.
 *
 * Aggregate struct — use C++20 designated initializers to set only the events you care about.
 * Unset handlers are default-constructed (falsy) and silently skipped by FlowEvents::dispatch.
 *
 * FltLoad and fltLoaded receive the path of the FLT file being loaded. All other events
 * carry no meaningful path; their handlers take no arguments.
 */
struct FlowEventHandlers {
    std::function<void(std::string_view)> fltLoad;        ///< Flight file starting to load.
    std::function<void(std::string_view)> fltLoaded;      ///< Flight file fully loaded.
    std::function<void()> teleportStart;                  ///< Teleport operation starting.
    std::function<void()> teleportDone;                   ///< Teleport operation complete.
    std::function<void()> backOnTrackStart;               ///< Back-on-track operation starting.
    std::function<void()> backOnTrackDone;                ///< Back-on-track operation complete.
    std::function<void()> skipStart;                      ///< Skip operation starting.
    std::function<void()> skipDone;                       ///< Skip operation complete.
    std::function<void()> backToMainMenu;                 ///< Returning to the main menu.
    std::function<void()> rtcStart;                       ///< Real-time communication starting.
    std::function<void()> rtcEnd;                         ///< Real-time communication ended.
    std::function<void()> replayStart;                    ///< Flight replay starting.
    std::function<void()> replayEnd;                      ///< Flight replay ended.
    std::function<void()> flightStart;                    ///< Flight starting.
    std::function<void()> flightEnd;                      ///< Flight ended.
    std::function<void()> planeCrash;                     ///< Aircraft crashed.
};

#else
#error "flow_event_handlers.hpp requires the MSFS 2024 SDK (Flow Events are not available in earlier SDK versions)."
#endif // MSFS_2024_SDK


} // namespace SimConnect
