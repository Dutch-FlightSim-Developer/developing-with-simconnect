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

#include <simconnect.hpp>


namespace SimConnect {


struct PeriodLimits {
    unsigned long origin{ 0 };
    unsigned long limit{ 0 };

    constexpr PeriodLimits() = default;
    constexpr PeriodLimits(unsigned long orig, unsigned long lim) : origin(orig), limit(lim) {}

    constexpr static PeriodLimits none() {
        return { 0, 0 };
    }
    constexpr static PeriodLimits startAfter(unsigned long origin)  noexcept {
        return { origin, 0 };
    }
    constexpr static PeriodLimits stopAfter(unsigned long limit) noexcept {
        return { 0, limit };
    }

    [[nodiscard]]
    constexpr unsigned long getOrigin() const noexcept {
        return origin;
    }
    [[nodiscard]]
    constexpr unsigned long getLimit() const noexcept {
        return limit;
    }

    constexpr PeriodLimits andStartAfter(unsigned long orig) const noexcept {
        return { orig, limit };
    }
    constexpr PeriodLimits andStopAfter(unsigned long lim) const noexcept {
        return { origin, lim };
    }
};

struct DataFrequency {
    int period{ SIMCONNECT_PERIOD_ONCE };
    unsigned long interval{ 0 };


    constexpr static DataFrequency once() noexcept {
        return { SIMCONNECT_PERIOD_ONCE, 0 };
    }
    constexpr static DataFrequency every(unsigned long intval) noexcept {
        return { SIMCONNECT_PERIOD_SECOND, intval };
    }
    constexpr static DataFrequency never() noexcept {
        return { SIMCONNECT_PERIOD_NEVER, 0 };
    }

    [[nodiscard]]
    constexpr SIMCONNECT_PERIOD getPeriod() const noexcept {
        return static_cast<SIMCONNECT_PERIOD>(period);
    }
    [[nodiscard]]
    constexpr operator SIMCONNECT_PERIOD() const noexcept {
        return getPeriod();
    }
    [[nodiscard]]
    constexpr DWORD getInterval() const noexcept {
        return interval;
    }


    [[nodiscard]]
    constexpr bool isOnce() const noexcept {
        return period == SIMCONNECT_PERIOD_ONCE;
    }

    constexpr DataFrequency seconds() const noexcept {
        return { SIMCONNECT_PERIOD_SECOND, interval };
    }
    constexpr DataFrequency visualFrames() const noexcept {
        return { SIMCONNECT_PERIOD_VISUAL_FRAME, interval };
    }
    constexpr DataFrequency simFrames() const noexcept {
        return { SIMCONNECT_PERIOD_SIM_FRAME, interval };
    }
};

} // namespace SimConnect