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

#include <simconnect/simconnect.hpp>

#include<functional>

namespace SimConnect {


enum class SimObjectTypeAsBitField : unsigned long {
    user            = 0b000000001,
    userAircraft    = user, // Alias for user
    aircraft        = 0b000000010,
    helicopter      = 0b000000100,
    boat            = 0b000001000,
    ground          = 0b000010000,
    hotAirBalloon   = 0b000100000,
    animal          = 0b001000000,
    userAvatar      = 0b010000000,
    userCurrent     = 0b100000000,
    all             = 0b111111111
};


/**
 * The SimObjectTypes class is a simple class that holds the types of SimObjects as a bitmask.
 * It is used by the requestDataByType methods to return the types of the SimObjects that the data was requested for.
 */
struct SimObjectTypeSet {
    unsigned long types_{ 0 };  ///< The bitmask of SimObject types.

    constexpr SimObjectTypeSet() = default;
	constexpr SimObjectTypeSet(SimObjectType type) {
        switch (type) {
        case SimObjectTypes::user:
        //case SimObjectType::userAircraft: // Alias for user
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::user);
            break;
        case SimObjectTypes::aircraft:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::aircraft);
            break;
        case SimObjectTypes::helicopter:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::helicopter);
            break;
        case SimObjectTypes::boat:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::boat);
            break;
        case SimObjectTypes::ground:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::ground);
            break;
        case SimObjectTypes::hotAirBalloon:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::hotAirBalloon);
            break;
        case SimObjectTypes::animal:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::animal);
            break;
        case SimObjectTypes::userAvatar:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::userAvatar);
            break;
        case SimObjectTypes::userCurrent:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::userCurrent);
            break;
        case SimObjectTypes::all:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::all);
            break;
        }
    }
    constexpr SimObjectTypeSet(SimObjectTypeAsBitField type) : types_(static_cast<unsigned long>(type)) {}
	constexpr SimObjectTypeSet(const SimObjectTypeSet&) = default;
    constexpr SimObjectTypeSet(SimObjectTypeSet&&) = default;
    constexpr SimObjectTypeSet& operator=(const SimObjectTypeSet&) = default;
    constexpr SimObjectTypeSet& operator=(SimObjectTypeSet&&) = default;

    constexpr static SimObjectTypeSet from(unsigned long types) noexcept {
        SimObjectTypeSet result;
        result.types_ = types;
        return result;
    }
    constexpr static SimObjectTypeSet user() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::user);
    }
    constexpr static SimObjectTypeSet userAircraft() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::userAircraft);
    }
    constexpr static SimObjectTypeSet aircraft() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::aircraft);
    }
    constexpr static SimObjectTypeSet helicopter() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::helicopter);
    }
    constexpr static SimObjectTypeSet boat() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::boat);
    }
    constexpr static SimObjectTypeSet ground() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::ground);
    }
    constexpr static SimObjectTypeSet hotAirBalloon() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::hotAirBalloon);
    }
    constexpr static SimObjectTypeSet animal() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::animal);
    }
    constexpr static SimObjectTypeSet userAvatar() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::userAvatar);
    }
    constexpr static SimObjectTypeSet userCurrent() noexcept {
        return SimObjectTypeSet(SimObjectTypeAsBitField::userCurrent);
    }


    constexpr operator unsigned long() const noexcept {
        return types_;
    }


    constexpr SimObjectTypeSet orUser() const noexcept {
        return SimObjectTypeSet::from(types_ | user());
    }
    constexpr SimObjectTypeSet orUserAircraft() const noexcept {
        return SimObjectTypeSet::from(types_ | userAircraft());
    }
    constexpr SimObjectTypeSet orAircraft() const noexcept {
        return SimObjectTypeSet::from(types_ | aircraft());
    }
    constexpr SimObjectTypeSet orHelicopter() const noexcept {
        return SimObjectTypeSet::from(types_ | helicopter());
    }
    constexpr SimObjectTypeSet orBoat() const noexcept {
        return SimObjectTypeSet::from(types_ | boat());
    }
    constexpr SimObjectTypeSet orGround() const noexcept {
        return SimObjectTypeSet::from(types_ | ground());
    }
    constexpr SimObjectTypeSet orHotAirBalloon() const noexcept {
        return SimObjectTypeSet::from(types_ | hotAirBalloon());
    }
    constexpr SimObjectTypeSet orAnimal() const noexcept {
        return SimObjectTypeSet::from(types_ | animal());
    }
    constexpr SimObjectTypeSet orUserAvatar() const noexcept {
        return SimObjectTypeSet::from(types_ | userAvatar());
    }
    constexpr SimObjectTypeSet orUserCurrent() const noexcept {
        return SimObjectTypeSet::from(types_ | userCurrent());
    }

    constexpr bool hasUser() const noexcept {
        return (types_ & user()) != 0;
	}
    constexpr bool hasUserAircraft() const noexcept {
		return (types_ & userAircraft()) != 0;
	}
    constexpr bool hasAircraft() const noexcept {
		return (types_ & aircraft()) != 0;
	}
	constexpr bool hasHelicopter() const noexcept {
		return (types_ & helicopter()) != 0;
	}
	constexpr bool hasBoat() const noexcept {
		return (types_ & boat()) != 0;
	}
	constexpr bool hasGround() const noexcept {
		return (types_ & ground()) != 0;
	}
	constexpr bool hasHotAirBalloon() const noexcept {
		return (types_ & hotAirBalloon()) != 0;
	}
	constexpr bool hasAnimal() const noexcept {
		return (types_ & animal()) != 0;
	}
	constexpr bool hasUserAvatar() const noexcept {
		return (types_ & userAvatar()) != 0;
	}
	constexpr bool hasUserCurrent() const noexcept {
		return (types_ & userCurrent()) != 0;
	}

    void forEach(const std::function<void(SimObjectType)>& func) const noexcept {
        if (hasUser()) func(SimObjectTypes::user);
        if (hasUserAircraft()) func(SimObjectTypes::userAircraft);
        if (hasAircraft()) func(SimObjectTypes::aircraft);
        if (hasHelicopter()) func(SimObjectTypes::helicopter);
        if (hasBoat()) func(SimObjectTypes::boat);
        if (hasGround()) func(SimObjectTypes::ground);
        if (hasHotAirBalloon()) func(SimObjectTypes::hotAirBalloon);
        if (hasAnimal()) func(SimObjectTypes::animal);
        if (hasUserAvatar()) func(SimObjectTypes::userAvatar);
        if (hasUserCurrent()) func(SimObjectTypes::userCurrent);
    }
};

} // namespace SimConnect