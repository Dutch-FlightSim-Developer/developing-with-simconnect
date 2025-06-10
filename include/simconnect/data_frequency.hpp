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


struct DataFrequency {
    SIMCONNECT_PERIOD period{ SIMCONNECT_PERIOD_ONCE };
    DWORD origin{ 0 };
    DWORD interval{ 0 };
    DWORD limit{ 0 };

    static DataFrequency once() {
        return { SIMCONNECT_PERIOD_ONCE, 0, 0, 0 };
    }
    static DataFrequency every(unsigned int interval) {
        return { SIMCONNECT_PERIOD_SECOND, 0, interval, 0 };
    }
    static DataFrequency never() {
        return { SIMCONNECT_PERIOD_NEVER, 0, 0, 0 };
    }

    DataFrequency& seconds() {
        period = SIMCONNECT_PERIOD_SECOND;
        return *this;
    }
    DataFrequency visualFrames() {
        period = SIMCONNECT_PERIOD_VISUAL_FRAME;
        return *this;
    }
    DataFrequency simFrames() {
        period = SIMCONNECT_PERIOD_SIM_FRAME;
        return *this;
    }
    DataFrequency skipFirst(unsigned int count) {
        origin = count;
        return *this;
    }
    DataFrequency stopAfter(unsigned int max) {
        limit = max;
        return *this;
    }
};

} // namespace SimConnect