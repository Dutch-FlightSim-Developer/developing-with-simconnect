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

 #include <cstdint>

 #include <array>
 #include <atomic>
 #include <string_view>


namespace SimConnect::Facilities {


inline constexpr std::size_t ShortNameLength = 4;
inline constexpr std::size_t NameLength = 32;
inline constexpr std::size_t Name64Length = 64;
inline constexpr std::size_t ICAOLength = 8;
inline constexpr std::size_t ShortRegionLength = 2;
inline constexpr std::size_t RegionLength = 8;
inline constexpr std::size_t CountryLength = 256;
inline constexpr std::size_t CityStateLength = 256;

inline constexpr double MetersToFeetFactor = 3.28084;  ///< Factor to convert meters to feet.
inline constexpr double FrequencyToKHzFactor = 0.001;    ///< Factor to convert Hz to KHz.
inline constexpr double FrequencyToMHzFactor = 0.000001;     ///< Factor to convert KHz to MHz.

/**
 * Return the next unique facility definition ID.
 * 
 * @returns The next unique facility definition ID.
 */
inline FacilityDefinitionId nextFacilityDefinitionId() {
    static std::atomic<FacilityDefinitionId> nextFacilityDefinitionId_{ 0 };

    return ++nextFacilityDefinitionId_;
}


#pragma pack(push, 1)

class MinimalFacilityData {
    char facilityType_;
    std::array<char, ICAOLength + 1> ident_;
    std::array<char, ShortRegionLength + 1> region_;
    std::array<char, ShortNameLength + 1> airport_;

    DataTypes::LatLonAlt location_;

public:
    constexpr char facilityType() const noexcept { return facilityType_; }
    constexpr bool isAirport() const noexcept { return (facilityType_ == 'A') || (facilityType_ == ' '); }
    constexpr bool isNDB() const noexcept { return facilityType_ == 'N'; }
    constexpr bool isVOR() const noexcept { return facilityType_ == 'V'; }
    constexpr bool isWaypoint() const noexcept { return facilityType_ == 'W'; }

    constexpr std::string_view ident() const noexcept { return { ident_.data(), ICAOLength }; }
    constexpr std::string_view region() const noexcept { return { region_.data(), ShortRegionLength }; }
    constexpr std::string_view airport() const noexcept { return { airport_.data(), ShortNameLength }; }

    const DataTypes::LatLonAlt& location() const noexcept { return location_; }
    constexpr double latitude() const noexcept { return location_.Latitude; }
    constexpr double longitude() const noexcept { return location_.Longitude; }
    constexpr double altitude() const noexcept { return location_.Altitude; }
};

#pragma pack(pop)

enum class ApproachLightsSystem : int32_t {
    None = 0,
    ODALS = 1,
    MALSF = 2,
    MALSR = 3,
    SSALF = 4,
    SSALR = 5,
    ALSF1 = 6,
    ALSF2 = 7,
    RAIL = 8,
    CALVERT = 9,
    CALVERT2 = 10,
    MALS = 11,
    SALS = 12,
    SALSF = 13,
    SSALS = 14,
};


enum class VASIType : int32_t {
    None = 0,
    VASI21 = 1,
    VASI22 = 2,
    VASI23 = 3,
    VASI31 = 4,
    VASI32 = 5,
    VASI33 = 6,
    PAPI2 = 7,
    PAPI4 = 8,
    TRICOLOR = 9,
    PVASI = 10,
    TVASI = 11,
    BALL = 12,
    APAP = 13,
};


enum class RunwaySurface : int32_t {
    Concrete = 0,    // CONCRETE
    Grass = 1,    // GRASS
    WaterFSX = 2,    // WATER FSX
    GrassBumpy = 3,    // GRASS BUMPY
    Asphalt = 4,    // ASPHALT
    ShortGrass = 5,    // SHORT GRASS
    LongGrass = 6,    // LONG GRASS
    HardTurf = 7,    // HARD TURF
    Snow = 8,    // SNOW
    Ice = 9,    // ICE
    Urban = 10,    // URBAN
    Forest = 11,    // FOREST
    Dirt = 12,    // DIRT
    Coral = 13,    // CORAL
    Gravel = 14,    // GRAVEL
    OilTreated = 15,    // OIL TREATED
    SteelMats = 16,    // STEEL MATS
    Bituminus = 17,    // BITUMINUS
    Brick = 18,    // BRICK
    Macadam = 19,    // MACADAM
    Planks = 20,    // PLANKS
    Sand = 21,    // SAND
    Shale = 22,    // SHALE
    Tarmac = 23,    // TARMAC
    WrightFlyerTrack = 24,    // WRIGHT FLYER TRACK
    Ocean = 26,    // OCEAN
    Water = 27,    // WATER
    Pond = 28,    // POND
    Lake = 29,    // LAKE
    River = 30,    // RIVER
    WasteWater = 31,    // WASTE WATER
    Paint = 32,    // PAINT
    Unknown = 254,    // UNKNOWN
    Undefined = 255,    // UNDEFINED
};


enum class IlsType : int32_t {
    None = 0,
    Airport     = static_cast<int32_t>('A'),
    VOR         = static_cast<int32_t>('V'),
    NDB         = static_cast<int32_t>('N'),
    Waypoint    = static_cast<int32_t>('W'),
};


enum class RunwayNumber : int32_t {
    None = 0,
    R01 =  1, R02 =  2, R03 =  3, R04 =  4, R05 =  5, R06 =  6, R07 =  7, R08 =  8, R09 =  9, R10 = 10,
    R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15, R16 = 16, R17 = 17, R18 = 18, R19 = 19, R20 = 20,
    R21 = 21, R22 = 22, R23 = 23, R24 = 24, R25 = 25, R26 = 26, R27 = 27, R28 = 28, R29 = 29, R30 = 30,
    R31 = 31, R32 = 32, R33 = 33, R34 = 34, R35 = 35, R36 = 36,
    North = 37,
    NorthEast = 38,
    East = 39,
    SouthEast = 40,
    South = 41,
    SouthWest = 42,
    West = 43,
    NorthWest = 44,
    Last = 45,
};


enum class RunwayDesignator : int32_t {
    None = 0,
    Left = 1,
    Right = 2,
    Center = 3,
    Water = 4,
    A = 5,
    B = 6,
    Last = 7,
};


enum class StartType : int32_t {
    None = 0,
    Runway = 1,
    Water = 2,
    Helipad = 3,
    Track = 4,
};


enum class TaxiParkingType : int32_t {
    None = 0,
    Ramp_GA = 1,
    Ramp_GA_Small = 2,
    Ramp_GA_Medium = 3,
    Ramp_GA_Large = 4,
    Ramp_Cargo = 5,
    Tamp_Mil_Cargo = 6,
    Ramp_Mil_Combat = 7,
    Gate_Small = 8,
    Gate_Medium = 9,
    Gate_Heavy = 10,
    Dock_GA = 11,
    Fuel = 12,
    Vehicle = 13,
    Ramp_GA_Extra = 14,
    Gate_Extra = 15,
};


enum class TaxiPointType : int32_t {
    None = 0,
    Normal = 1,
    HoldShort = 2,
    IlsHoldShort = 4,
    HoldShortNoDraw = 5,
    IlsHoldShortNoDraw = 6,
};


enum class ParkingName : int32_t {
    None = 0,
    Parking = 1,
    N_Parking = 2,
    NE_Parking = 3,
    E_Parking = 4,
    SE_Parking = 5,
    S_Parking = 6,
    SW_Parking = 7,
    W_Parking = 8,
    NW_Parking = 9,
    Gate = 10,
    Dock = 11,
    Gate_A = 12, Gate_B = 13, Gate_C = 14, Gate_D = 15, Gate_E = 16, Gate_F = 17,
    Gate_G = 18, Gate_H = 19, Gate_I = 20, Gate_J = 21, Gate_K = 22, Gate_L = 23,
    Gate_M = 24, Gate_N = 25, Gate_O = 26, Gate_P = 27, Gate_Q = 28, Gate_R = 29,
    Gate_S = 30, Gate_T = 31, Gate_U = 32, Gate_V = 33, Gate_W = 34, Gate_X = 35,
    Gate_Y = 36, Gate_Z = 37,
};


enum class ParkingOrientation : int32_t {
    Forward = 0,
    Reverse = 1,
};


enum class FrequencyType : int32_t {
    None = 0,
    ATIS = 1,
    Multicom = 2,
    Unicom = 3,
    CTAF = 4,
    Ground = 5,
    Tower = 6,
    Clearance = 7,
    Approach = 8,
    Departure = 9,
    Center = 10,
    FSS = 11,
    AWOS = 12,
    ASOS = 13,
    CPT = 14,
    GCO = 15,
};


enum class VORType : int32_t {
    VOR_Unknown = 0,
    VOR_Terminal = 1,
    VOR_LowAltitude = 2,
    VOR_HighAltitude = 3,
    VOR_ILS = 4,
    VOR_VOT = 5,
};


enum class LocalizerCategory : int32_t {
    None = 0,
    Cat1 = 1,
    Cat2 = 2,
    Cat3 = 3,
    Localizer = 4,
    IGS = 5,
    LDA_NoGS = 6,
    LDA_WithGS = 7,
    SDF_NoGS = 8,
    SDF_WithGS = 9,
};

#pragma region Helipad data structures

enum class HelipadType : int32_t {
    None = 0,
    H = 1,
    Square = 2,
    Circle = 3,
    Medical = 4,
};


class HelipadData {
    double latitude_;       // LATITUDE
    double longitude_;      // LONGITUDE
    double altitude_;       // ALTITUDE
    float heading_;         // HEADING
    float length_;          // LENGTH
    float width_;           // WIDTH
    RunwaySurface surface_;   // SURFACE
    HelipadType type_;      // TYPE
    float touchDownLength_; // TOUCH_DOWN_LENGTH
    float fatoLength_;      // FATO_LENGTH
    float fatoWidth_;       // FATO_WIDTH

public:
    constexpr double latitude() const noexcept { return latitude_; }
    constexpr double longitude() const noexcept { return longitude_; }
    constexpr double altitude() const noexcept { return altitude_; }
    constexpr float heading() const noexcept { return heading_; }
    constexpr float length() const noexcept { return length_; }
    constexpr float width() const noexcept { return width_; }
    constexpr RunwaySurface surface() const noexcept { return surface_; }
    constexpr HelipadType type() const noexcept { return type_; }
    constexpr float touchDownLength() const noexcept { return touchDownLength_; }
    constexpr float fatoLength() const noexcept { return fatoLength_; }
    constexpr float fatoWidth() const noexcept { return fatoWidth_; }
};

#pragma endregion

#pragma region Approach data structures

enum class ApproachType : int32_t {
    None = 0,
    GPS = 1,
    VOR = 2,
    NDB = 3,
    ILS = 4,
    Localizer = 5,
    SDF = 6,
    LDA = 7,
    VOR_DME = 8,
    NDB_DME = 9,
    RNAV = 10,
    Localizer_BackCourse = 11,
};

using FafType = IlsType;


class ApproachData {
    ApproachType type_;        // TYPE
    int32_t suffix_;           // SUFFIX
    RunwayNumber runwayNumber_;      // RUNWAY_NUMBER
    RunwayDesignator runwayDesignator_;  // RUNWAY_DESIGNATOR

    std::array<char, ICAOLength> fafIcao_; // FAF_ICAO
    std::array<char, RegionLength> fafRegion_; // FAF_REGION
    float fafHeading_;         // FAF_HEADING
    float fafAltitude_;        // FAF_ALTITUDE
    FafType fafType_;          // FAF_TYPE

    float missedAltitude_;     // MISSED_ALTITUDE
    int32_t hasLnav_;           // HAS_LNAV
    int32_t hasLnavVnav_;       // HAS_LNAVVNAV
    int32_t hasLp_;             // HAS_LP
    int32_t hasLpv_;            // HAS_LPV
    int32_t isRnpAr_;           // IS_RNPAR
    int32_t isRnpArMissed_;     // IS_RNPAR_MISSED
    int32_t nTransitions_;     // N_TRANSITIONS
    int32_t nFinalApproachLegs_; // N_FINAL_APPROACH_LEGS
    int32_t nMissedApproachLegs_; // N_MISSED_APPROACH_LEGS

public:
    constexpr ApproachType type() const noexcept { return type_; }
    constexpr char suffix() const noexcept { return (suffix_ == 0) ? ' ' : static_cast<char>(suffix_); }
    constexpr RunwayNumber runwayNumber() const noexcept { return runwayNumber_; }
    constexpr RunwayDesignator runwayDesignator() const noexcept { return runwayDesignator_; }

    constexpr std::string_view fafIcao() const noexcept { return { fafIcao_.data(), ICAOLength }; }
    constexpr std::string_view fafRegion() const noexcept { return { fafRegion_.data(), RegionLength }; }
    constexpr float fafHeading() const noexcept { return fafHeading_; }
    constexpr float fafAltitude() const noexcept { return fafAltitude_; }
    constexpr FafType fafType() const noexcept { return fafType_; }

    constexpr float missedAltitude() const noexcept { return missedAltitude_; }
    constexpr bool hasLnav() const noexcept { return hasLnav_ != 0; }
    constexpr bool hasLnavVnav() const noexcept { return hasLnavVnav_ != 0; }
    constexpr bool hasLp() const noexcept { return hasLp_ != 0; }
    constexpr bool hasLpv() const noexcept { return hasLpv_ != 0; }
    constexpr bool isRnpAr() const noexcept { return isRnpAr_ != 0; }
    constexpr bool isRnpArMissed() const noexcept { return isRnpArMissed_ != 0; }
    constexpr int32_t nTransitions() const noexcept { return nTransitions_; }
    constexpr int32_t nFinalApproachLegs() const noexcept { return nFinalApproachLegs_; }
    constexpr int32_t nMissedApproachLegs() const noexcept { return nMissedApproachLegs_; }
};

#pragma endregion

#pragma region ApproachTransition data structures


using IafType = IlsType;


class ApproachTransitionData {
    ApproachType type_;        // TYPE

    std::array<char, ICAOLength> iafIcao_; // IAF_ICAO
    std::array<char, RegionLength> iafRegion_; // IAF_REGION
    IafType iafType_;          // IAF_TYPE
    float iafAltitude_;        // IAF_ALTITUDE

    std::array<char, ICAOLength> dmeArcIcao_; // DME_ARC_ICAO
    std::array<char, RegionLength> dmeArcRegion_; // DME_ARC_REGION
    float dmeArcRadial_;      // DME_ARC_RADIAL
    float dmeArcDistance_;    // DME_ARC_DISTANCE

    std::array<char, NameLength> name_; // NAME
    int32_t nApproachLegs_;   // N_APPROACH_LEGS

public:
    constexpr std::string_view name() const noexcept { return { name_.data(), NameLength }; }
    constexpr ApproachType type() const noexcept { return type_; }
    constexpr int32_t nApproachLegs() const noexcept { return nApproachLegs_; }

    constexpr std::string_view iafIcao() const noexcept { return { iafIcao_.data(), ICAOLength }; }
    constexpr std::string_view iafRegion() const noexcept { return { iafRegion_.data(), RegionLength }; }
    constexpr IafType iafType() const noexcept { return iafType_; }
    constexpr float iafAltitude() const noexcept { return iafAltitude_; }

    constexpr std::string_view dmeArcIcao() const noexcept { return { dmeArcIcao_.data(), ICAOLength }; }
    constexpr std::string_view dmeArcRegion() const noexcept { return { dmeArcRegion_.data(), RegionLength }; }
    constexpr float dmeArcRadial() const noexcept { return dmeArcRadial_; }
    constexpr float dmeArcDistance() const noexcept { return dmeArcDistance_; }
};

#pragma endregion

#pragma region ApproachLeg data structures

enum class LegType : int32_t {
    None = 0,
    AF = 1, // DME Arc to Fix
    CA = 2, // Course to Altitude
    CD = 3, // Course to DME Distance
    CF = 4, // Course to Fix
    CI = 5, // Course to Intercept
    CR = 6, // Course to Radial
    DF = 7, // Direct to Fix
    FA = 8, // Fix to Altitude
    FC = 9, // Track from Fix
    FD = 10, // Track from Fix to DME Distance
    FM = 11, // Track from Fix to Manual terminator
    HA = 12, // Racetrack course reversal to Altitude
    HF = 13, // Racetrack course reversal to Fix
    HM = 14, // Racetrack course reversal to Manual terminator
    IF = 15, // Initial Fix
    PI = 16, // Procedure turn
    RF = 17, // Constant Radius Arc
    TF = 18, // Track to Fix
    VA = 19, // Vector to Altitude
    VD = 20, // Heading to DME Distance
    VI = 21, // Heading to Intercept
    VM = 22, // Heading to Manual terminator
    VR = 23, // Heading to Radial
};

using FixType = IlsType;
using OriginType = IlsType;

enum class TurnDirection : int32_t {
    None = 0,
    Left = 1,
    Right = 2,
    Either = 3,
};

enum class ApproachAlternateDescription : int32_t {
    NotUsed = 0,
    At = 1,
    AtOrAbove = 2,
    AtOrBelow = 3,
    InBetween = 4,
};

enum class ApproachSpeedDescription : int32_t {
    None = 0,
    At = 1,
    AtOrAbove = 2,
    AtOrBelow = 3,
};


class ApproachLegData {
    LegType type_;            // TYPE

    std::array<char, ICAOLength> fixIcao_; // FIX_ICAO
    std::array<char, RegionLength> fixRegion_; // FIX_REGION
    FixType fixType_;          // FIX_TYPE
    double fixLatitude_;      // FIX_LATITUDE
    double fixLongitude_;     // FIX_LONGITUDE
    double fixAltitude_;        // FIX_ALTITUDE

    int32_t flyOver_;         // FLY_OVER
    int32_t distanceMinute_;   // DISTANCE_MINUTE
    int32_t trueDegree_;      // TRUE_DEGREE
    TurnDirection turnDirection_; // TURN_DIRECTION

    std::array<char, ICAOLength> originIcao_; // ORIGIN_ICAO
    std::array<char, RegionLength> originRegion_; // ORIGIN_REGION
    OriginType originType_;    // ORIGIN_TYPE
    double originLatitude_;    // ORIGIN_LATITUDE
    double originLongitude_;   // ORIGIN_LONGITUDE
    double originAltitude_;     // ORIGIN_ALTITUDE

    float theta_;            // THETA
    float rho_;              // RHO
    float course_;           // COURSE
    float routeDistance_;    // ROUTE_DISTANCE
    ApproachAlternateDescription approachAltDesc_; // APPROACH_ALT_DESC
    float altitude1_;        // ALTITUDE1
    float altitude2_;        // ALTITUDE2
    float speedLimit_;       // SPEED_LIMIT
    float verticalAngle_;     // VERTICAL_ANGLE

    std::array<char, ICAOLength> arcCenterFixIcao_; // ARC_CENTER_FIX_ICAO
    std::array<char, RegionLength> arcCenterFixRegion_; // ARC_CENTER_FIX_REGION
    FixType arcCenterFixType_; // ARC_CENTER_FIX_TYPE
    double arcCenterFixLatitude_; // ARC_CENTER_FIX_LATITUDE
    double arcCenterFixLongitude_; // ARC_CENTER_FIX_LONGITUDE
    double arcCenterFixAltitude_; // ARC_CENTER_FIX_ALTITUDE
    float radius_;            // RADIUS
    int32_t isIAF_;          // IS_IAF
    int32_t isIF_;          // IS_IF
    int32_t isFAF_;         // IS_FAF
    int32_t isMAP_;         // IS_MAP

    float requiredNavigationPerformance_; // REQUIRED_NAVIGATION_PERFORMANCE
    ApproachSpeedDescription approachSpeedDesc_; // APPROACH_SPEED_DESC

public:
    constexpr LegType type() const noexcept { return type_; }

    constexpr std::string_view fixIcao() const noexcept { return { fixIcao_.data(), ICAOLength }; }
    constexpr std::string_view fixRegion() const noexcept { return { fixRegion_.data(), RegionLength }; }
    constexpr FixType fixType() const noexcept { return fixType_; }
    constexpr double fixLatitude() const noexcept { return fixLatitude_; }
    constexpr double fixLongitude() const noexcept { return fixLongitude_; }
    constexpr double fixAltitude() const noexcept { return fixAltitude_; }
    constexpr bool isFlyOver() const noexcept { return flyOver_ != 0; }
    constexpr bool distanceInMinute() const noexcept { return distanceMinute_ != 0; }
    constexpr bool trueDegrees() const noexcept { return trueDegree_ != 0; }
    constexpr TurnDirection turnDirection() const noexcept { return turnDirection_; }

    constexpr std::string_view originIcao() const noexcept { return { originIcao_.data(), ICAOLength }; }
    constexpr std::string_view originRegion() const noexcept { return { originRegion_.data(), RegionLength }; }
    constexpr OriginType originType() const noexcept { return originType_; }
    constexpr double originLatitude() const noexcept { return originLatitude_; }
    constexpr double originLongitude() const noexcept { return originLongitude_; }
    constexpr double originAltitude() const noexcept { return originAltitude_; }

    constexpr float theta() const noexcept { return theta_; }
    constexpr float rho() const noexcept { return rho_; }
    constexpr float course() const noexcept { return course_; }
    constexpr float routeDistance() const noexcept { return routeDistance_; }
    constexpr ApproachAlternateDescription approachAltDesc() const noexcept { return approachAltDesc_; }
    constexpr float altitude1() const noexcept { return altitude1_; }
    constexpr float altitude2() const noexcept { return altitude2_; }
    constexpr float speedLimit() const noexcept { return speedLimit_; }
    constexpr float verticalAngle() const noexcept { return verticalAngle_; }

    constexpr std::string_view arcCenterFixIcao() const noexcept { return { arcCenterFixIcao_.data(), ICAOLength }; }
    constexpr std::string_view arcCenterFixRegion() const noexcept { return { arcCenterFixRegion_.data(), RegionLength }; }
    constexpr FixType arcCenterFixType() const noexcept { return arcCenterFixType_; }
    constexpr double arcCenterFixLatitude() const noexcept { return arcCenterFixLatitude_; }
    constexpr double arcCenterFixLongitude() const noexcept { return arcCenterFixLongitude_; }
    constexpr double arcCenterFixAltitude() const noexcept { return arcCenterFixAltitude_; }
    constexpr float radius() const noexcept { return radius_; }

    constexpr bool isIAF() const noexcept { return isIAF_ != 0; }
    constexpr bool isIF() const noexcept { return isIF_ != 0; }
    constexpr bool isFAF() const noexcept { return isFAF_ != 0; }
    constexpr bool isMAP() const noexcept { return isMAP_ != 0; }

    constexpr float requiredNavigationPerformance() const noexcept { return requiredNavigationPerformance_; }
    constexpr ApproachSpeedDescription approachSpeedDesc() const noexcept { return approachSpeedDesc_; }
};

#pragma endregion


} // namespace SimConnect::Facilities