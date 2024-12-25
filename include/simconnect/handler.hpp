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


#include <simconnect/connection.hpp>

#include <chrono>
#include <array>
#include <functional>


namespace SimConnect {

typedef std::function<void(SIMCONNECT_RECV*, DWORD)> HandlerProc;


template <class C, class T>
class Handler
{
private:
    std::array<HandlerProc, SIMCONNECT_RECV_ID_ENUMERATE_SIMOBJECT_AND_LIVERY_LIST+1> handlers_;
    HandlerProc defaultHandler_;

protected:
    C& connection_;

    Handler(C& connection) : connection_(connection) {};

    void dispatch(SIMCONNECT_RECV* msg, DWORD size) {
        if (handlers_[msg->dwID]) {
            handlers_[msg->dwID](msg, size);
        }
        else if (defaultHandler_) {
            defaultHandler_(msg, size);
        }
    }

    void dispatchWaitingMessages() {
        SIMCONNECT_RECV* msg = nullptr;
        DWORD size = 0;

        while (SUCCEEDED(SimConnect_GetNextDispatch(connection_, &msg, &size))) {
            dispatch(msg, size);
        }
    }

public:
    void setDefaultHandler(HandlerProc proc) { defaultHandler_ = proc; }
    void registerHandler(SIMCONNECT_RECV_ID id, HandlerProc proc) { handlers_[id] = proc; }

    void handle(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) { static_cast<T*>(this)->dispatch(duration); }
};

}
