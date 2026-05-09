#pragma once
/*
 * Copyright (c) 2026. Bert Laverman
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

#include <simconnect/simconnect.hpp>
#include <simconnect/data_frequency.hpp>
#include <simconnect/requests/client_data_handler.hpp>


namespace SimConnect {


/**
 * Encapsulates a SimConnect client data area that is both read and written by the same client.
 *
 * Combines the three concerns that every bidirectional area consumer must wire by hand:
 *   - Mapping the area name to a ClientDataId
 *   - Owning the RawClientDataDefinition used for both subscribe and send
 *   - Managing the echo subscription and a user-supplied "ready to send" predicate
 *
 * **Ready predicate**
 *
 * `send()` checks a caller-supplied `ReadyFn` against the local state before writing.
 * The state is initialised to `T{}` and is updated from the echo on every receive.
 * After each successful `send()` the state is also updated optimistically, so a second
 * call before the echo returns will correctly see the intermediate state and return false.
 *
 * Use the default (`[](const T&) { return true; }`) for fire-and-forget areas (e.g.
 * a shared ping-pong blackboard) and a field-checking lambda for handshake protocols:
 *
 * @code
 *   // Handshake (PMDG control area): wait for plugin to reset Event to 0
 *   BidirectionalClientDataArea<B737NGControl, Handler> area(
 *       dataHandler, B737NGControlName,
 *       [](const B737NGControl& s) { return s.Event == 0; });
 *
 *   // Fire-and-forget (ping-pong shared blackboard):
 *   BidirectionalClientDataArea<PingPongData, Handler> area(dataHandler, "My.Area");
 * @endcode
 *
 * **Ownership**
 *
 * The area must outlive all dispatched messages. It must not be copied or moved
 * because the echo subscription lambda captures `this`.
 *
 * **Setup order**
 *
 * 1. Construct after `connection.open()` (name mapping requires an open handle).
 * 2. If this client owns the area: call `dataHandler.createClientData(area.id(), sizeof(T))`.
 * 3. Optionally register an `onEcho()` observer.
 * 4. Call `subscribe()`.
 * 5. Call `send()` as needed.
 *
 * @tparam T          Struct type transferred as a raw binary blob.
 * @tparam MsgHandler SimConnect message handler type (e.g. `WindowsEventHandler<…>`).
 */
template <typename T, typename MsgHandler>
class BidirectionalClientDataArea
{
public:
    using ReadyFn = std::function<bool(const T&)>;

    BidirectionalClientDataArea(const BidirectionalClientDataArea&) = delete;
    BidirectionalClientDataArea(BidirectionalClientDataArea&&)      = delete;
    BidirectionalClientDataArea& operator=(const BidirectionalClientDataArea&) = delete;
    BidirectionalClientDataArea& operator=(BidirectionalClientDataArea&&)      = delete;


    /**
     * Map the client data area name and store the ready predicate.
     *
     * Requires an open SimConnect connection — construct after `connection.open()`.
     *
     * @param dataHandler  The client data handler owning the connection and subscriptions.
     * @param name         The SimConnect client data area name (null-terminated).
     * @param readyFn      Predicate called against local state before each send.
     *                     Defaults to always-ready (fire-and-forget).
     */
    explicit BidirectionalClientDataArea(
        ClientDataHandler<MsgHandler>& dataHandler,
        std::string_view               name,
        ReadyFn                        readyFn = [](const T&) { return true; })
        : dataHandler_(dataHandler)
        , areaId_(dataHandler.mapClientDataName(name))
        , readyFn_(std::move(readyFn))
    {}

    ~BidirectionalClientDataArea() = default;


    /**
     * The mapped ClientDataId. Use this to call `dataHandler.createClientData()` if
     * this client owns the area.
     */
    [[nodiscard]]
    ClientDataId id() const noexcept { return areaId_; }


    /**
     * Register an observer called on every echo received from the area.
     * Must be called before `subscribe()`.
     *
     * @param cb  Called with the received struct on every update.
     */
    void onEcho(std::function<void(const T&)> cb) { onEcho_ = std::move(cb); }


    /**
     * Define the data layout with SimConnect and subscribe to echo notifications.
     *
     * @param frequency  How often SimConnect delivers updates. Default: on every write.
     * @param limits     Period count limits. Default: no limit.
     */
    void subscribe(
        ClientDataFrequency frequency = ClientDataFrequency::onSet(),
        PeriodLimits        limits    = PeriodLimits::none())
    {
        echoReq_ = dataHandler_.requestClientData(areaId_, def_,
            [this](const T& echo) {
                state_ = echo;
                if (onEcho_) { onEcho_(echo); }
            },
            frequency, limits);
    }


    /**
     * Write data to the area if the ready predicate allows it.
     *
     * Updates local state optimistically before sending, so a second call before the
     * echo returns will see the intermediate state and return false.
     *
     * @param data  The value to write.
     * @returns     true if the data was written; false if the predicate returned false.
     */
    bool send(const T& data) {
        if (!readyFn_(state_)) { return false; }
        state_ = data;
        dataHandler_.sendClientData(areaId_, def_, state_);
        return true;
    }


private:
    ClientDataHandler<MsgHandler>& dataHandler_;
    ClientDataId                   areaId_;
    RawClientDataDefinition<T>     def_;
    T                              state_{};
    ReadyFn                        readyFn_;
    std::function<void(const T&)>  onEcho_;
    Request                        echoReq_;
};


} // namespace SimConnect
