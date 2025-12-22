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
    DataPeriod period{ DataPeriods::once };
    unsigned long interval{ 0 };


    constexpr static DataFrequency once() noexcept {
        return { DataPeriods::once, 0 };
    }
    constexpr static DataFrequency every(unsigned long intval) noexcept {
        return { DataPeriods::second, intval };
    }
    constexpr static DataFrequency never() noexcept {
        return { DataPeriods::never, 0 };
    }

    [[nodiscard]]
    constexpr DataPeriod getPeriod() const noexcept {
        return period;
    }
    [[nodiscard]]
    constexpr operator DataPeriod() const noexcept {
        return getPeriod();
    }
    [[nodiscard]]
    constexpr unsigned long getInterval() const noexcept {
        return interval;
    }


    [[nodiscard]]
    constexpr bool isOnce() const noexcept {
        return period == DataPeriods::once;
    }

    constexpr DataFrequency seconds() const noexcept {
        return { DataPeriods::second, interval };
    }
    constexpr DataFrequency visualFrames() const noexcept {
        return { DataPeriods::visualFrame, interval };
    }
    constexpr DataFrequency simFrames() const noexcept {
        return { DataPeriods::simFrame, interval };
    }
};

} // namespace SimConnect