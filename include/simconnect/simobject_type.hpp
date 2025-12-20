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

#include <simconnect.hpp>

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
struct SimObjectTypes {
    unsigned long types_{ 0 };  ///< The bitmask of SimObject types.

    constexpr SimObjectTypes() = default;
    constexpr SimObjectTypes(unsigned long types) : types_(types) {}
	constexpr SimObjectTypes(SIMCONNECT_SIMOBJECT_TYPE type) {
        switch (type) {
        case SIMCONNECT_SIMOBJECT_TYPE_USER:
        //case SIMCONNECT_SIMOBJECT_TYPE_USER_AIRCRAFT:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::user);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::aircraft);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::helicopter);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_BOAT:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::boat);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_GROUND:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::ground);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_HOT_AIR_BALLOON:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::hotAirBalloon);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_ANIMAL:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::animal);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_USER_AVATAR:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::userAvatar);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_USER_CURRENT:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::userCurrent);
            break;
        case SIMCONNECT_SIMOBJECT_TYPE_ALL:
            types_ = static_cast<unsigned long>(SimObjectTypeAsBitField::all);
            break;
        }
    }
    constexpr SimObjectTypes(SimObjectTypeAsBitField type) : types_(static_cast<unsigned long>(type)) {}
	SimObjectTypes(const SimObjectTypes&) = default;
    SimObjectTypes(SimObjectTypes&&) = default;
    SimObjectTypes& operator=(const SimObjectTypes&) = default;
    SimObjectTypes& operator=(SimObjectTypes&&) = default;

    constexpr static SimObjectTypes user() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::user);
    }
    constexpr static SimObjectTypes userAircraft() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::userAircraft);
    }
    constexpr static SimObjectTypes aircraft() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::aircraft);
    }
    constexpr static SimObjectTypes helicopter() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::helicopter);
    }
    constexpr static SimObjectTypes boat() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::boat);
    }
    constexpr static SimObjectTypes ground() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::ground);
    }
    constexpr static SimObjectTypes hotAirBalloon() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::hotAirBalloon);
    }
    constexpr static SimObjectTypes animal() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::animal);
    }
    constexpr static SimObjectTypes userAvatar() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::userAvatar);
    }
    constexpr static SimObjectTypes userCurrent() noexcept {
        return SimObjectTypes(SimObjectTypeAsBitField::userCurrent);
    }


    constexpr operator unsigned long() const noexcept {
        return types_;
    }


    constexpr SimObjectTypes orUser() const noexcept {
        return SimObjectTypes(types_ | user());
    }
    constexpr SimObjectTypes orUserAircraft() const noexcept {
        return SimObjectTypes(types_ | userAircraft());
    }
    constexpr SimObjectTypes orAircraft() const noexcept {
        return SimObjectTypes(types_ | aircraft());
    }
    constexpr SimObjectTypes orHelicopter() const noexcept {
        return SimObjectTypes(types_ | helicopter());
    }
    constexpr SimObjectTypes orBoat() const noexcept {
        return SimObjectTypes(types_ | boat());
    }
    constexpr SimObjectTypes orGround() const noexcept {
        return SimObjectTypes(types_ | ground());
    }
    constexpr SimObjectTypes orHotAirBalloon() const noexcept {
        return SimObjectTypes(types_ | hotAirBalloon());
    }
    constexpr SimObjectTypes orAnimal() const noexcept {
        return SimObjectTypes(types_ | animal());
    }
    constexpr SimObjectTypes orUserAvatar() const noexcept {
        return SimObjectTypes(types_ | userAvatar());
    }
    constexpr SimObjectTypes orUserCurrent() const noexcept {
        return SimObjectTypes(types_ | userCurrent());
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

    void forEach(const std::function<void(SIMCONNECT_SIMOBJECT_TYPE)>& func) const noexcept {
        if (hasUser()) func(SIMCONNECT_SIMOBJECT_TYPE_USER);
        if (hasUserAircraft()) func(SIMCONNECT_SIMOBJECT_TYPE_USER_AIRCRAFT);
        if (hasAircraft()) func(SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
        if (hasHelicopter()) func(SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);
        if (hasBoat()) func(SIMCONNECT_SIMOBJECT_TYPE_BOAT);
        if (hasGround()) func(SIMCONNECT_SIMOBJECT_TYPE_GROUND);
        if (hasHotAirBalloon()) func(SIMCONNECT_SIMOBJECT_TYPE_HOT_AIR_BALLOON);
        if (hasAnimal()) func(SIMCONNECT_SIMOBJECT_TYPE_ANIMAL);
        if (hasUserAvatar()) func(SIMCONNECT_SIMOBJECT_TYPE_USER_AVATAR);
        if (hasUserCurrent()) func(SIMCONNECT_SIMOBJECT_TYPE_USER_CURRENT);
    }
};


/**
 * The SimObjectType class is a simple class that holds the type of a SimObject.
 * It is used by the requestDataByType methods to return the type of the SimObject that the data was requested for.
 */
struct SimObjectType {
    int typeId{ 0 };	///< The type ID of the SimObject.

    constexpr SimObjectType() = default;
    constexpr SimObjectType(int id) : typeId(id) {}
    constexpr SimObjectType(SIMCONNECT_SIMOBJECT_TYPE id) : typeId(static_cast<int>(id)) {}
	SimObjectType(const SimObjectType&) = default;
    SimObjectType(SimObjectType&&) = default;
    SimObjectType& operator=(const SimObjectType&) = default;
    SimObjectType& operator=(SimObjectType&&) = default;

    constexpr static SimObjectType user() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_USER);
    }
    constexpr static SimObjectType userAircraft() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_USER_AIRCRAFT);
    }
    constexpr static SimObjectType aircraft() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
    }
    constexpr static SimObjectType helicopter() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);
    }
    constexpr static SimObjectType boat() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_BOAT);
    }
    constexpr static SimObjectType ground() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_GROUND);
    }
    constexpr static SimObjectType hotAirBalloon() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_HOT_AIR_BALLOON);
    }
    constexpr static SimObjectType animal() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_ANIMAL);
    }
    constexpr static SimObjectType userAvatar() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_USER_AVATAR);
    }
    constexpr static SimObjectType userCurrent() noexcept {
        return SimObjectType(SIMCONNECT_SIMOBJECT_TYPE_USER_CURRENT);
    }


    constexpr operator SIMCONNECT_SIMOBJECT_TYPE() const noexcept {
        return static_cast<SIMCONNECT_SIMOBJECT_TYPE>(typeId);
    }

    constexpr SimObjectTypes orUser() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orUser();
    }
    constexpr SimObjectTypes orUserAircraft() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orUserAircraft();
    }
    constexpr SimObjectTypes orAircraft() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orAircraft();
    }
    constexpr SimObjectTypes orHelicopter() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orHelicopter();
    }
    constexpr SimObjectTypes orBoat() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orBoat();
    }
    constexpr SimObjectTypes orGround() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orGround();
    }
    constexpr SimObjectTypes orHotAirBalloon() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orHotAirBalloon();
    }
    constexpr SimObjectTypes orAnimal() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orAnimal();
    }
    constexpr SimObjectTypes orUserAvatar() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orUserAvatar();
    }
    constexpr SimObjectTypes orUserCurrent() const noexcept {
        return SimObjectTypes(SimObjectType(typeId)).orUserCurrent();
    }
};

} // namespace SimConnect