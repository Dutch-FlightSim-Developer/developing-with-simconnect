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


/**
 * The PeriodLimits struct represents limits on the period of a data request, such as starting after
 * a certain time or stopping after a certain time. This is used for requests that have a DataPeriod
 * of SIMCONNECT_PERIOD_SECOND, where the interval specifies the number of seconds between requests,
 * and the PeriodLimits can specify when to start and stop the requests.
 */
struct PeriodLimits {
    unsigned long origin{ 0 };
    unsigned long limit{ 0 };

    constexpr PeriodLimits() = default;
    constexpr PeriodLimits(unsigned long orig, unsigned long lim) : origin(orig), limit(lim) {}

    // Rule of five
    constexpr PeriodLimits(const PeriodLimits&) = default;
    constexpr PeriodLimits(PeriodLimits&&) = default;
    constexpr PeriodLimits& operator=(const PeriodLimits&) = default;
    constexpr PeriodLimits& operator=(PeriodLimits&&) = default;


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

    [[nodiscard]]
    constexpr PeriodLimits andStartAfter(unsigned long orig) const noexcept {
        return { orig, limit };
    }
    [[nodiscard]]
    constexpr PeriodLimits andStopAfter(unsigned long lim) const noexcept {
        return { origin, lim };
    }
};


/**
 * The DataFrequency struct represents the frequency at which SimObject data is requested.
 * It consists of a DataPeriod, which is the base frequency, and an interval, which is used
 * for certain DataPeriods to specify the number of periods between requests (e.g., every 5 seconds).
 */
struct DataFrequency {
    DataPeriod period{ DataPeriods::once };
    unsigned long interval{ 0 };


    constexpr static DataFrequency once() noexcept {
        return { DataPeriods::once, 0 };
    }
    constexpr static DataFrequency every(unsigned long intval = 1) noexcept {
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


    [[nodiscard]]
    constexpr DataFrequency second() const noexcept {
        return { DataPeriods::second, interval };
    }
    [[nodiscard]]
    constexpr DataFrequency seconds() const noexcept {
        return { DataPeriods::second, interval };
    }
    [[nodiscard]]
    constexpr DataFrequency visualFrame() const noexcept {
        return { DataPeriods::visualFrame, interval };
    }
    [[nodiscard]]
    constexpr DataFrequency visualFrames() const noexcept {
        return { DataPeriods::visualFrame, interval };
    }
    [[nodiscard]]
    constexpr DataFrequency simFrame() const noexcept {
        return { DataPeriods::simFrame, interval };
    }
    [[nodiscard]]
    constexpr DataFrequency simFrames() const noexcept {
        return { DataPeriods::simFrame, interval };
    }
};


/**
 * The ClientDataFrequency struct represents the frequency at which Client Data is requested.
 * It consists of a ClientDataPeriod and an interval.
 */
struct ClientDataFrequency {
    ClientDataPeriod period{ ClientDataPeriods::once };
    unsigned long interval{ 0 };


    constexpr static ClientDataFrequency once() noexcept {
        return { ClientDataPeriods::once, 0 };
    }
    constexpr static ClientDataFrequency every(unsigned long intval = 1) noexcept {
        return { ClientDataPeriods::second, intval };
    }
    constexpr static ClientDataFrequency onSet() noexcept {
        return { ClientDataPeriods::onSet, 0 };
    }
    constexpr static ClientDataFrequency never() noexcept {
        return { ClientDataPeriods::never, 0 };
    }

    [[nodiscard]]
    constexpr ClientDataPeriod getPeriod() const noexcept {
        return period;
    }
    [[nodiscard]]
    constexpr operator ClientDataPeriod() const noexcept {
        return getPeriod();
    }
    [[nodiscard]]
    constexpr unsigned long getInterval() const noexcept {
        return interval;
    }

    [[nodiscard]]
    constexpr bool isOnce() const noexcept {
        return period == ClientDataPeriods::once;
    }

    [[nodiscard]]
    constexpr ClientDataFrequency second() const noexcept {
        return { ClientDataPeriods::second, interval };
    }
    [[nodiscard]]
    constexpr ClientDataFrequency seconds() const noexcept {
        return { ClientDataPeriods::second, interval };
    }
    [[nodiscard]]
    constexpr ClientDataFrequency visualFrame() const noexcept {
        return { ClientDataPeriods::visualFrame, interval };
    }
    [[nodiscard]]
    constexpr ClientDataFrequency visualFrames() const noexcept {
        return { ClientDataPeriods::visualFrame, interval };
    }
};


inline constexpr bool onlyWhenChanged{ true };
inline constexpr bool always{ false };

} // namespace SimConnect