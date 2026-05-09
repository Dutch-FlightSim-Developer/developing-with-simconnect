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

#include <cstddef>
#include <optional>
#include <concepts>
#include <functional>
#include <type_traits>

#include <simconnect/simconnect.hpp>


namespace SimConnect {


/**
 * Satisfied by any type that can serve as a client data definition for a given connection.
 *
 * Requires:
 *   - `d.id()`             — returns a value convertible to `ClientDataDefinitionId`
 *   - `d.define(connection)` — registers with SimConnect and returns `D&` for chaining
 *
 * @tparam D              The definition type (e.g. `RawClientDataDefinition<T>`).
 * @tparam ConnectionType The SimConnect connection type passed to `define()`.
 */
template <typename D, typename ConnectionType>
concept ClientDataDefinitionConcept = requires(D& d, ConnectionType& connection)
{
    { d.id() } -> std::convertible_to<ClientDataDefinitionId>;
    { d.define(connection) } -> std::same_as<D&>;
};

/**
 * Satisfied by any callable that accepts a `const S&` datum and returns void.
 *
 * Accepts lambdas, `std::function`, and any other callable matching the signature.
 *
 * @tparam F The callable type.
 * @tparam S The struct type delivered by the client data area.
 */
template <typename F, typename S>
concept ClientDataHandlerConcept = requires(F f, const S& s)
{
    { std::invoke(f, s) } -> std::same_as<void>;
};


/**
 * CRTP base for all client data definition types.
 *
 * Manages the `ClientDataDefinitionId` lifecycle and total wire size.
 * Derived classes implement `registerFields(connection)` to register
 * their datums with SimConnect via `addClientDataDefinition()`.
 *
 * Satisfies `ClientDataDefinitionConcept` for any connection type — this is
 * what allows derived types to be passed directly to `requestClientData()`.
 *
 * Every `Derived` must implement:
 *   - `void registerFields(connection_type&)` — called once by `define()`
 *
 * @tparam Derived  The concrete definition type (CRTP).
 */
template <typename Derived>
class ClientDataDefinitionBase
{
    std::optional<ClientDataDefinitionId> id_{ std::nullopt };

protected:
    size_t size_{ 0 };

public:
    [[nodiscard]]
    bool isDefined() const noexcept { return id_.has_value(); }

    [[nodiscard]]
    ClientDataDefinitionId id() const noexcept {
        return id_.value_or(static_cast<ClientDataDefinitionId>(noId));
    }

    operator ClientDataDefinitionId() const noexcept { return id(); }

    /**
     * Total wire size of all registered datums in bytes.
     * Accumulated by derived classes as fields are added.
     */
    [[nodiscard]]
    size_t size() const noexcept { return size_; }

    /**
     * Register this definition with SimConnect, allocating a new ClientDataDefinitionId.
     * Idempotent: subsequent calls are no-ops.
     *
     * @returns Reference to the Derived object for chaining.
     */
    template <class connection_type>
    Derived& define(connection_type& connection) {
        if (!isDefined()) {
            id_ = connection.clientDataDefinitions().nextDataDefID();
            static_cast<Derived*>(this)->registerFields(connection);
        }
        return static_cast<Derived&>(*this);
    }
};


} // namespace SimConnect
