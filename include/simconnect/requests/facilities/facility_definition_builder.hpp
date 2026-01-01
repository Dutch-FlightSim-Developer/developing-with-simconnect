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

#include <array>


#include <simconnect/simconnect.hpp>


namespace SimConnect::Facilities {


enum class FacilityField {
    /* Airports */
    airportOpen = 0,
    airportClose,
    airportLatitude,
    airportLongitude,
    airportAltitude,
    airportMagvar,
    airportName,
    airportName64,
    airportICAO,
    airportRegion,
    airportTowerLatitude,
    airportTowerLongitude,
    airportTowerAltitude,
    airportTransitionAltitude,
    airportTransitionLevel,
    airportIsClosed,
    airportCountry,
    airportCityState,
    airportRunways,
    airportStarts,
    airportFrequencies,
    airportHelipads,
    airportApproaches,
    airportDepartures,
    airportArrivals,
    airportTaxiPoints,
    airportTaxiParkings,
    airportTaxiPaths,
    airportTaxiNames,
    airportJetways,
    airportVDGS,
    airportHoldingPatterns,

    /* Airport child: Runways */
    runwayOpen,
    runwayClose,
    runwayLatitude,
    runwayLongitude,
    runwayAltitude,
    runwayHeading,
    runwayLength,
    runwayWidth,
    runwayPatternAltitude,
    runwaySlope,
    runwayTrueSlope,
    runwaySurface,
    runwayPrimaryILSICAO,
    runwayPrimaryILSRegion,
    runwayPrimaryILSType,
    runwayPrimaryNumber,
    runwayPrimaryDesignator,
    runwayPrimaryThreshold,
    runwayPrimaryBlastpad,
    runwayPrimaryOverrun,
    runwayPrimaryApproachLights,
    runwayPrimaryLeftVASI,
    runwayPrimaryRightVASI,
    runwaySecondaryILSICAO,
    runwaySecondaryILSRegion,
    runwaySecondaryILSType,
    runwaySecondaryNumber,
    runwaySecondaryDesignator,
    runwayEdgeLights,
    runwayCenterLights,
    runwayPrimaryClosed,
    runwaySecondaryClosed,
    runwayPrimaryTakeoff,
    runwayPrimaryLanding,
    runwaySecondaryTakeoff,
    runwaySecondaryLanding,
    runwaySecondaryThreshold,
    runwaySecondaryBlastpad,
    runwaySecondaryOverrun,
    runwaySecondaryApproachLights,
    runwaySecondaryLeftVASI,
    runwaySecondaryRightVASI,

    /* Runway child: Pavement */
    pavementOpen,
    pavementClose,
    pavementLength,
    pavementWidth,
    pavementEnable,

    /* Runway child: ApproachLights */
    approachLightsOpen,
    approachLightsClose,
    approachLightsSystem,
    approachLightsStrobeCount,
    approachLightsHasEndLights,
    approachLightsHasReilLights,
    approachLightsHasTouchdownLights,
    approachLightsOnGround,
    approachLightsEnable,
    approachLightsOffset,
    approachLightsSpacing,
    approachLightsSlope,

    /* Runway child: VASI */
    vasiOpen,
    vasiClose,
    vasiType,
    vasiBiasX,
    vasiBiasZ,
    vasiSpacing,
    vasiAngle,

    /* Airport child: Start */
    startOpen,
    startClose,
    startLatitude,
    startLongitude,
    startAltitude,
    startHeading,
    startNumber,
    startDesignator,
    startType,

    /* Airport child: Frequency */
    frequencyOpen,
    frequencyClose,
    frequencyType,
    frequencyFrequency,
    frequencyName,

    /* Airport child: Helipad */
    helipadOpen,
    helipadClose,
    helipadLatitude,
    helipadLongitude,
    helipadAltitude,
    helipadHeading,
    helipadLength,
    helipadWidth,
    helipadSurface,
    helipadType,
    helipadTouchDownLength,
    helipadFatoLength,
    helipadFatoWidth,

    /* Airport child: Approach */
    approachOpen,
    approachClose,
    approachType,
    approachSuffix,
    approachRunwayNumber,
    approachRunwayDesignator,
    approachFafICAO,
    approachFafRegion,
    approachFafHeading,
    approachFafAltitude,
    approachFafType,
    approachMissedAltitude,
    approachHasLnav,
    approachHasLnavVnav,
    approachHasLP,
    approachHasLPV,
    approachIsRnpAR,
    approachIsRnpARMissed,
    approachNTransitions,
    approachNFinalApproachLegs,
    approachNMissedApproachLegs,

    /* Approach child: ApproachTransition */
    approachTransitionOpen,
    approachTransitionClose,
    approachTransitionType,
    approachTransitionIafICAO,
    approachTransitionIafRegion,
    approachTransitionIafType,
    approachTransitionIafAltitude,
    approachTransitionDmeArcICAO,
    approachTransitionDmeArcRegion,
    approachTransitionDmeArcType,
    approachTransitionDmeArcRadial,
    approachTransitionDmeArcDistance,
    approachTransitionName,
    approachTransitionNApproachLegs,

    /* Approach/Transition child: ApproachLeg / FinalApproachLeg / MissedApproachLeg */
    approachLegOpen,
    approachLegClose,
    approachLegType,
    approachLegFixICAO,
    approachLegFixRegion,
    approachLegFixType,
    approachLegFixLatitude,
    approachLegFixLongitude,
    approachLegFixAltitude,
    approachLegFlyOver,
    approachLegDistanceMinute,
    approachLegTrueDegree,
    approachLegTurnDirection,
    approachLegOriginICAO,
    approachLegOriginRegion,
    approachLegOriginType,
    approachLegOriginLatitude,
    approachLegOriginLongitude,
    approachLegOriginAltitude,
    approachLegTheta,
    approachLegRho,
    approachLegCourse,
    approachLegRouteDistance,
    approachLegApproachAltDesc,
    approachLegAltitude1,
    approachLegAltitude2,
    approachLegSpeedLimit,
    approachLegVerticalAngle,
    approachLegArcCenterFixICAO,
    approachLegArcCenterFixRegion,
    approachLegArcCenterFixType,
    approachLegArcCenterFixLatitude,
    approachLegArcCenterFixLongitude,
    approachLegArcCenterFixAltitude,
    approachLegRadius,
    approachLegIsIAF,
    approachLegIsIF,
    approachLegIsFAF,
    approachLegIsMAP,
    approachLegRequiredNavigationPerformance,
    approachLegApproachSpeedDesc,

    /* Approach child: FinalApproachLeg (uses APPROACH_LEG fields) */
    finalApproachLegOpen,
    finalApproachLegClose,

    /* Approach child: MissedApproachLeg (uses APPROACH_LEG fields) */
    missedApproachLegOpen,
    missedApproachLegClose,

    /* Airport child: Departure */
    departureOpen,
    departureClose,
    departureName,
    departureIsRnpAR,
    departureNRunwayTransitions,
    departureNEnrouteTransitions,
    departureNApproachLegs,

    /* Departure/Arrival child: RunwayTransition */
    runwayTransitionOpen,
    runwayTransitionClose,
    runwayTransitionRunwayNumber,
    runwayTransitionRunwayDesignator,
    runwayTransitionNApproachLegs,

    /* Departure/Arrival child: EnrouteTransition */
    enrouteTransitionOpen,
    enrouteTransitionClose,
    enrouteTransitionName,
    enrouteTransitionNApproachLegs,

    /* Airport child: Arrival */
    arrivalOpen,
    arrivalClose,
    arrivalName,
    arrivalIsRnpAR,
    arrivalNRunwayTransitions,
    arrivalNEnrouteTransitions,
    arrivalNApproachLegs,

    /* Airport child: TaxiParking */
    taxiParkingOpen,
    taxiParkingClose,
    taxiParkingType,
    taxiParkingTaxiPointType,
    taxiParkingName,
    taxiParkingSuffix,
    taxiParkingNumber,
    taxiParkingOrientation,
    taxiParkingHeading,
    taxiParkingRadius,
    taxiParkingBiasX,
    taxiParkingBiasZ,
    taxiParkingNAirlines,

    /* TaxiParking child: Airline */
    airlineOpen,
    airlineClose,
    airlineName,

    /* Airport child: TaxiPoint */
    taxiPointOpen,
    taxiPointClose,
    taxiPointType,
    taxiPointOrientation,
    taxiPointBiasX,
    taxiPointBiasZ,

    /* Airport child: TaxiPath */
    taxiPathOpen,
    taxiPathClose,
    taxiPathType,
    taxiPathWidth,
    taxiPathLeftHalfWidth,
    taxiPathRightHalfWidth,
    taxiPathWeight,
    taxiPathRunwayNumber,
    taxiPathRunwayDesignator,
    taxiPathLeftEdge,
    taxiPathLeftEdgeLighted,
    taxiPathRightEdge,
    taxiPathRightEdgeLighted,
    taxiPathCenterLine,
    taxiPathCenterLineLighted,
    taxiPathStart,
    taxiPathEnd,
    taxiPathNameIndex,

    /* Airport child: TaxiName */
    taxiNameOpen,
    taxiNameClose,
    taxiNameName,

    /* Airport child: Jetway */
    jetwayOpen,
    jetwayClose,
    jetwayParkingGate,
    jetwayParkingSuffix,
    jetwayParkingSpot,

    /* Airport child: VDGS */
    vdgsOpen,
    vdgsClose,
    vdgsLatitude,
    vdgsLongitude,
    vdgsAltitude,
    vdgsParkingNumber,
    vdgsParkingGate,
    vdgsParkingSuffix,
    vdgsParkingIndex,

    /* Airport child: HoldingPattern */
    holdingPatternOpen,
    holdingPatternClose,
    holdingPatternName,
    holdingPatternFixICAO,
    holdingPatternFixRegion,
    holdingPatternFixType,
    holdingPatternInboundHoldingCourse,
    holdingPatternTurnRight,
    holdingPatternLegLength,
    holdingPatternLegTime,
    holdingPatternMinAltitude,
    holdingPatternMaxAltitude,
    holdingPatternHoldSpeed,
    holdingPatternRequiredNavigationPerformance,
    holdingPatternArcRadius,

    /* Waypoints */
    waypointOpen,
    waypointClose,
    waypointLatitude,
    waypointLongitude,
    waypointAltitude,
    waypointType,
    waypointMagvar,
    waypointNRoutes,
    waypointICAO,
    waypointRegion,
    waypointIsTerminalWpt,

    /* Waypoint child: Route */
    routeOpen,
    routeClose,
    routeName,
    routeType,
    routeNextICAO,
    routeNextRegion,
    routeNextType,
    routeNextLatitude,
    routeNextLongitude,
    routeNextAltitude,
    routePrevICAO,
    routePrevRegion,
    routePrevType,
    routePrevLatitude,
    routePrevLongitude,
    routePrevAltitude,

    /* NDB */
    ndbOpen,
    ndbClose,
    ndbLatitude,
    ndbLongitude,
    ndbAltitude,
    ndbFrequency,
    ndbType,
    ndbRange,
    ndbMagvar,
    ndbIsTerminalNDB,
    ndbName,
    ndbBfoRequired,

    /* VOR */
    vorOpen,
    vorClose,
    vorVorLatitude,
    vorVorLongitude,
    vorVorAltitude,
    vorDmeLatitude,
    vorDmeLongitude,
    vorDmeAltitude,
    vorGsLatitude,
    vorGsLongitude,
    vorGsAltitude,
    vorTacanLatitude,
    vorTacanLongitude,
    vorTacanAltitude,
    vorIsNav,
    vorIsDme,
    vorIsTacan,
    vorHasGlideSlope,
    vorDmeAtNav,
    vorDmeAtGlideSlope,
    vorHasBackCourse,
    vorFrequency,
    vorType,
    vorNavRange,
    vorMagvar,
    vorLocalizer,
    vorLocalizerWidth,
    vorGlideSlope,
    vorName,
    vorDmeBias,
    vorLsCategory,
    vorIsTrueReferenced,
};

struct FacilityFieldInfo {
    const char* name;
    DataType type;
    std::size_t size;
};

inline constexpr std::array<FacilityFieldInfo, static_cast<std::size_t>(FacilityField::vorIsTrueReferenced) + 1> facilityFieldInfos = {{
    /* Airports */
    {"OPEN AIRPORT", DataTypes::invalid, 0},
    {"CLOSE AIRPORT", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"MAGVAR", DataTypes::float32, 4},
    {"NAME", DataTypes::string32, 32},
    {"NAME64", DataTypes::string64, 64},
    {"ICAO", DataTypes::string8, 8},
    {"REGION", DataTypes::string8, 8},
    {"TOWER_LATITUDE", DataTypes::float64, 8},
    {"TOWER_LONGITUDE", DataTypes::float64, 8},
    {"TOWER_ALTITUDE", DataTypes::float64, 8},
    {"TRANSITION_ALTITUDE", DataTypes::float32, 4},
    {"TRANSITION_LEVEL", DataTypes::float32, 4},
    {"IS_CLOSED", DataTypes::int8, 1},
    {"COUNTRY", DataTypes::string256, 256},
    {"CITY_STATE", DataTypes::string256, 256},
    {"N_RUNWAYS", DataTypes::int32, 4},
    {"N_STARTS", DataTypes::int32, 4},
    {"N_FREQUENCIES", DataTypes::int32, 4},
    {"N_HELIPADS", DataTypes::int32, 4},
    {"N_APPROACHES", DataTypes::int32, 4},
    {"N_DEPARTURES", DataTypes::int32, 4},
    {"N_ARRIVALS", DataTypes::int32, 4},
    {"N_TAXI_POINTS", DataTypes::int32, 4},
    {"N_TAXI_PARKINGS", DataTypes::int32, 4},
    {"N_TAXI_PATHS", DataTypes::int32, 4},
    {"N_TAXI_NAMES", DataTypes::int32, 4},
    {"N_JETWAYS", DataTypes::int32, 4},
    {"N_VDGS", DataTypes::int32, 4},
    {"N_HOLDING_PATTERNS", DataTypes::int32, 4},

    /* Airport child: Runways */
    {"OPEN RUNWAY", DataTypes::invalid, 0},
    {"CLOSE RUNWAY", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"HEADING", DataTypes::float32, 4},
    {"LENGTH", DataTypes::float32, 4},
    {"WIDTH", DataTypes::float32, 4},
    {"PATTERN_ALTITUDE", DataTypes::float32, 4},
    {"SLOPE", DataTypes::float32, 4},
    {"TRUE_SLOPE", DataTypes::float32, 4},
    {"SURFACE", DataTypes::int32, 4},
    {"PRIMARY_ILS_ICAO", DataTypes::string8, 8},
    {"PRIMARY_ILS_REGION", DataTypes::string8, 8},
    {"PRIMARY_ILS_TYPE", DataTypes::int32, 4},
    {"PRIMARY_NUMBER", DataTypes::int32, 4},
    {"PRIMARY_DESIGNATOR", DataTypes::int32, 4},
    {"PRIMARY_THRESHOLD", DataTypes::invalid, 0},
    {"PRIMARY_BLASTPAD", DataTypes::invalid, 0},
    {"PRIMARY_OVERRUN", DataTypes::invalid, 0},
    {"PRIMARY_APPROACH_LIGHTS", DataTypes::invalid, 0},
    {"PRIMARY_LEFT_VASI", DataTypes::invalid, 0},
    {"PRIMARY_RIGHT_VASI", DataTypes::invalid, 0},
    {"SECONDARY_ILS_ICAO", DataTypes::string8, 8},
    {"SECONDARY_ILS_REGION", DataTypes::string8, 8},
    {"SECONDARY_ILS_TYPE", DataTypes::int32, 4},
    {"SECONDARY_NUMBER", DataTypes::int32, 4},
    {"SECONDARY_DESIGNATOR", DataTypes::int32, 4},
    {"EDGE_LIGHTS", DataTypes::int8, 1},
    {"CENTER_LIGHTS", DataTypes::int8, 1},
    {"PRIMARY_CLOSED", DataTypes::int8, 1},
    {"SECONDARY_CLOSED", DataTypes::int8, 1},
    {"PRIMARY_TAKEOFF", DataTypes::int8, 1},
    {"PRIMARY_LANDING", DataTypes::int8, 1},
    {"SECONDARY_TAKEOFF", DataTypes::int8, 1},
    {"SECONDARY_LANDING", DataTypes::int8, 1},
    {"SECONDARY_THRESHOLD", DataTypes::invalid, 0},
    {"SECONDARY_BLASTPAD", DataTypes::invalid, 0},
    {"SECONDARY_OVERRUN", DataTypes::invalid, 0},
    {"SECONDARY_APPROACH_LIGHTS", DataTypes::invalid, 0},
    {"SECONDARY_LEFT_VASI", DataTypes::invalid, 0},
    {"SECONDARY_RIGHT_VASI", DataTypes::invalid, 0},

    /* Runway child: Pavement */
    {"OPEN PAVEMENT", DataTypes::invalid, 0},
    {"CLOSE PAVEMENT", DataTypes::invalid, 0},
    {"LENGTH", DataTypes::float32, 4},
    {"WIDTH", DataTypes::float32, 4},
    {"ENABLE", DataTypes::int32, 4},

    /* Runway child: ApproachLights */
    {"OPEN APPROACHLIGHTS", DataTypes::invalid, 0},
    {"CLOSE APPROACHLIGHTS", DataTypes::invalid, 0},
    {"SYSTEM", DataTypes::int32, 4},
    {"STROBE_COUNT", DataTypes::int32, 4},
    {"HAS_END_LIGHTS", DataTypes::int32, 4},
    {"HAS_REIL_LIGHTS", DataTypes::int32, 4},
    {"HAS_TOUCHDOWN_LIGHTS", DataTypes::int32, 4},
    {"ON_GROUND", DataTypes::int32, 4},
    {"ENABLE", DataTypes::int32, 4},
    {"OFFSET", DataTypes::float32, 4},
    {"SPACING", DataTypes::float32, 4},
    {"SLOPE", DataTypes::float32, 4},

    /* Runway child: VASI */
    {"OPEN VASI", DataTypes::invalid, 0},
    {"CLOSE VASI", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"BIAS_X", DataTypes::float32, 4},
    {"BIAS_Z", DataTypes::float32, 4},
    {"SPACING", DataTypes::float32, 4},
    {"ANGLE", DataTypes::float32, 4},

    /* Airport child: Start */
    {"OPEN START", DataTypes::invalid, 0},
    {"CLOSE START", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"HEADING", DataTypes::float32, 4},
    {"NUMBER", DataTypes::int32, 4},
    {"DESIGNATOR", DataTypes::int32, 4},
    {"TYPE", DataTypes::int32, 4},

    /* Airport child: Frequency */
    {"OPEN FREQUENCY", DataTypes::invalid, 0},
    {"CLOSE FREQUENCY", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"FREQUENCY", DataTypes::int32, 4},
    {"NAME", DataTypes::string64, 64},

    /* Airport child: Helipad */
    {"OPEN HELIPAD", DataTypes::invalid, 0},
    {"CLOSE HELIPAD", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"HEADING", DataTypes::float32, 4},
    {"LENGTH", DataTypes::float32, 4},
    {"WIDTH", DataTypes::float32, 4},
    {"SURFACE", DataTypes::int32, 4},
    {"TYPE", DataTypes::int32, 4},
    {"TOUCH_DOWN_LENGTH", DataTypes::float32, 4},
    {"FATO_LENGTH", DataTypes::float32, 4},
    {"FATO_WIDTH", DataTypes::float32, 4},

    /* Airport child: Approach */
    {"OPEN APPROACH", DataTypes::invalid, 0},
    {"CLOSE APPROACH", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"SUFFIX", DataTypes::int32, 4},
    {"RUNWAY_NUMBER", DataTypes::int32, 4},
    {"RUNWAY_DESIGNATOR", DataTypes::int32, 4},
    {"FAF_ICAO", DataTypes::string8, 8},
    {"FAF_REGION", DataTypes::string8, 8},
    {"FAF_HEADING", DataTypes::float32, 4},
    {"FAF_ALTITUDE", DataTypes::float32, 4},
    {"FAF_TYPE", DataTypes::int32, 4},
    {"MISSED_ALTITUDE", DataTypes::float32, 4},
    {"HAS_LNAV", DataTypes::int32, 4},
    {"HAS_LNAVVNAV", DataTypes::int32, 4},
    {"HAS_LP", DataTypes::int32, 4},
    {"HAS_LPV", DataTypes::int32, 4},
    {"IS_RNPAR", DataTypes::int32, 4},
    {"IS_RNPAR_MISSED", DataTypes::int32, 4},
    {"N_TRANSITIONS", DataTypes::int32, 4},
    {"N_FINAL_APPROACH_LEGS", DataTypes::int32, 4},
    {"N_MISSED_APPROACH_LEGS", DataTypes::int32, 4},

    /* Approach child: ApproachTransition */
    {"OPEN APPROACH_TRANSITION", DataTypes::invalid, 0},
    {"CLOSE APPROACH_TRANSITION", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"IAF_ICAO", DataTypes::string8, 8},
    {"IAF_REGION", DataTypes::string8, 8},
    {"IAF_TYPE", DataTypes::int32, 4},
    {"IAF_ALTITUDE", DataTypes::float32, 4},
    {"DME_ARC_ICAO", DataTypes::string8, 8},
    {"DME_ARC_REGION", DataTypes::string8, 8},
    {"DME_ARC_TYPE", DataTypes::int32, 4},
    {"DME_ARC_RADIAL", DataTypes::int32, 4},
    {"DME_ARC_DISTANCE", DataTypes::float32, 4},
    {"NAME", DataTypes::string8, 8},
    {"N_APPROACH_LEGS", DataTypes::int32, 4},

    /* Approach/Transition child: ApproachLeg / FinalApproachLeg / MissedApproachLeg */
    {"OPEN APPROACH_LEG", DataTypes::invalid, 0},
    {"CLOSE APPROACH_LEG", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"FIX_ICAO", DataTypes::string8, 8},
    {"FIX_REGION", DataTypes::string8, 8},
    {"FIX_TYPE", DataTypes::int32, 4},
    {"FIX_LATITUDE", DataTypes::float64, 8},
    {"FIX_LONGITUDE", DataTypes::float64, 8},
    {"FIX_ALTITUDE", DataTypes::float64, 8},
    {"FLY_OVER", DataTypes::int32, 4},
    {"DISTANCE_MINUTE", DataTypes::int32, 4},
    {"TRUE_DEGREE", DataTypes::int32, 4},
    {"TURN_DIRECTION", DataTypes::int32, 4},
    {"ORIGIN_ICAO", DataTypes::string8, 8},
    {"ORIGIN_REGION", DataTypes::string8, 8},
    {"ORIGIN_TYPE", DataTypes::int32, 4},
    {"ORIGIN_LATITUDE", DataTypes::float64, 8},
    {"ORIGIN_LONGITUDE", DataTypes::float64, 8},
    {"ORIGIN_ALTITUDE", DataTypes::float64, 8},
    {"THETA", DataTypes::float32, 4},
    {"RHO", DataTypes::float32, 4},
    {"COURSE", DataTypes::float32, 4},
    {"ROUTE_DISTANCE", DataTypes::float32, 4},
    {"APPROACH_ALT_DESC", DataTypes::int32, 4},
    {"ALTITUDE1", DataTypes::float32, 4},
    {"ALTITUDE2", DataTypes::float32, 4},
    {"SPEED_LIMIT", DataTypes::float32, 4},
    {"VERTICAL_ANGLE", DataTypes::float32, 4},
    {"ARC_CENTER_FIX_ICAO", DataTypes::string8, 8},
    {"ARC_CENTER_FIX_REGION", DataTypes::string8, 8},
    {"ARC_CENTER_FIX_TYPE", DataTypes::int32, 4},
    {"ARC_CENTER_FIX_LATITUDE", DataTypes::float64, 8},
    {"ARC_CENTER_FIX_LONGITUDE", DataTypes::float64, 8},
    {"ARC_CENTER_FIX_ALTITUDE", DataTypes::float64, 8},
    {"RADIUS", DataTypes::float32, 4},
    {"IS_IAF", DataTypes::int32, 4},
    {"IS_IF", DataTypes::int32, 4},
    {"IS_FAF", DataTypes::int32, 4},
    {"IS_MAP", DataTypes::int32, 4},
    {"REQUIRED_NAVIGATION_PERFORMANCE", DataTypes::float32, 4},
    {"APPROACH_SPEED_DESC", DataTypes::int32, 4},

    /* Approach child: FinalApproachLeg (uses APPROACH_LEG fields) */
    {"OPEN FINAL_APPROACH_LEG", DataTypes::invalid, 0},
    {"CLOSE FINAL_APPROACH_LEG", DataTypes::invalid, 0},

    /* Approach child: MissedApproachLeg (uses APPROACH_LEG fields) */
    {"OPEN MISSED_APPROACH_LEG", DataTypes::invalid, 0},
    {"CLOSE MISSED_APPROACH_LEG", DataTypes::invalid, 0},

    /* Airport child: Departure */
    {"OPEN DEPARTURE", DataTypes::invalid, 0},
    {"CLOSE DEPARTURE", DataTypes::invalid, 0},
    {"NAME", DataTypes::string8, 8},
    {"IS_RNPAR", DataTypes::int32, 4},
    {"N_RUNWAY_TRANSITIONS", DataTypes::int32, 4},
    {"N_ENROUTE_TRANSITIONS", DataTypes::int32, 4},
    {"N_APPROACH_LEGS", DataTypes::int32, 4},

    /* Departure/Arrival child: RunwayTransition */
    {"OPEN RUNWAY_TRANSITION", DataTypes::invalid, 0},
    {"CLOSE RUNWAY_TRANSITION", DataTypes::invalid, 0},
    {"RUNWAY_NUMBER", DataTypes::int32, 4},
    {"RUNWAY_DESIGNATOR", DataTypes::int32, 4},
    {"N_APPROACH_LEGS", DataTypes::int32, 4},

    /* Departure/Arrival child: EnrouteTransition */
    {"OPEN ENROUTE_TRANSITION", DataTypes::invalid, 0},
    {"CLOSE ENROUTE_TRANSITION", DataTypes::invalid, 0},
    {"NAME", DataTypes::string8, 8},
    {"N_APPROACH_LEGS", DataTypes::int32, 4},

    /* Airport child: Arrival */
    {"OPEN ARRIVAL", DataTypes::invalid, 0},
    {"CLOSE ARRIVAL", DataTypes::invalid, 0},
    {"NAME", DataTypes::string8, 8},
    {"IS_RNPAR", DataTypes::int32, 4},
    {"N_RUNWAY_TRANSITIONS", DataTypes::int32, 4},
    {"N_ENROUTE_TRANSITIONS", DataTypes::int32, 4},
    {"N_APPROACH_LEGS", DataTypes::int32, 4},

    /* Airport child: TaxiParking */
    {"OPEN TAXI_PARKING", DataTypes::invalid, 0},
    {"CLOSE TAXI_PARKING", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"TAXI_POINT_TYPE", DataTypes::int32, 4},
    {"NAME", DataTypes::int32, 4},
    {"SUFFIX", DataTypes::int32, 4},
    {"NUMBER", DataTypes::int32, 4},
    {"ORIENTATION", DataTypes::int32, 4},
    {"HEADING", DataTypes::float32, 4},
    {"RADIUS", DataTypes::float32, 4},
    {"BIAS_X", DataTypes::float32, 4},
    {"BIAS_Z", DataTypes::float32, 4},
    {"N_AIRLINES", DataTypes::int32, 4},

    /* TaxiParking child: Airline */
    {"OPEN AIRLINE", DataTypes::invalid, 0},
    {"CLOSE AIRLINE", DataTypes::invalid, 0},
    {"NAME", DataTypes::string8, 8},

    /* Airport child: TaxiPoint */
    {"OPEN TAXI_POINT", DataTypes::invalid, 0},
    {"CLOSE TAXI_POINT", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"ORIENTATION", DataTypes::int32, 4},
    {"BIAS_X", DataTypes::float32, 4},
    {"BIAS_Z", DataTypes::float32, 4},

    /* Airport child: TaxiPath */
    {"OPEN TAXI_PATH", DataTypes::invalid, 0},
    {"CLOSE TAXI_PATH", DataTypes::invalid, 0},
    {"TYPE", DataTypes::int32, 4},
    {"WIDTH", DataTypes::float32, 4},
    {"LEFT_HALF_WIDTH", DataTypes::float32, 4},
    {"RIGHT_HALF_WIDTH", DataTypes::float32, 4},
    {"WEIGHT", DataTypes::int32, 4},
    {"RUNWAY_NUMBER", DataTypes::int32, 4},
    {"RUNWAY_DESIGNATOR", DataTypes::int32, 4},
    {"LEFT_EDGE", DataTypes::int32, 4},
    {"LEFT_EDGE_LIGHTED", DataTypes::int32, 4},
    {"RIGHT_EDGE", DataTypes::int32, 4},
    {"RIGHT_EDGE_LIGHTED", DataTypes::int32, 4},
    {"CENTER_LINE", DataTypes::int32, 4},
    {"CENTER_LINE_LIGHTED", DataTypes::int32, 4},
    {"START", DataTypes::int32, 4},
    {"END", DataTypes::int32, 4},
    {"NAME_INDEX", DataTypes::int32, 4},

    /* Airport child: TaxiName */
    {"OPEN TAXI_NAME", DataTypes::invalid, 0},
    {"CLOSE TAXI_NAME", DataTypes::invalid, 0},
    {"NAME", DataTypes::string32, 32},

    /* Airport child: Jetway */
    {"OPEN JETWAY", DataTypes::invalid, 0},
    {"CLOSE JETWAY", DataTypes::invalid, 0},
    {"PARKING_GATE", DataTypes::int32, 4},
    {"PARKING_SUFFIX", DataTypes::int32, 4},
    {"PARKING_SPOT", DataTypes::int32, 4},

    /* Airport child: VDGS */
    {"OPEN VDGS", DataTypes::invalid, 0},
    {"CLOSE VDGS", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"PARKING_NUMBER", DataTypes::int32, 4},
    {"PARKING_GATE", DataTypes::int32, 4},
    {"PARKING_SUFFIX", DataTypes::int32, 4},
    {"PARKING_INDEX", DataTypes::int32, 4},

    /* Airport child: HoldingPattern */
    {"OPEN HOLDING_PATTERN", DataTypes::invalid, 0},
    {"CLOSE HOLDING_PATTERN", DataTypes::invalid, 0},
    {"NAME", DataTypes::string64, 64},
    {"FIX_ICAO", DataTypes::string8, 8},
    {"FIX_REGION", DataTypes::string8, 8},
    {"FIX_TYPE", DataTypes::int32, 4},
    {"INBOUND_HOLDING_COURSE", DataTypes::float32, 4},
    {"TURN_RIGHT", DataTypes::int32, 4},
    {"LEG_LENGTH", DataTypes::float32, 4},
    {"LEG_TIME", DataTypes::float32, 4},
    {"MIN_ALTITUDE", DataTypes::float32, 4},
    {"MAX_ALTITUDE", DataTypes::float32, 4},
    {"HOLD_SPEED", DataTypes::float32, 4},
    {"REQUIRED_NAVIGATION_PERFORMANCE", DataTypes::float32, 4},
    {"ARC_RADIUS", DataTypes::float32, 4},

    /* Waypoints */
    {"OPEN WAYPOINT", DataTypes::invalid, 0},
    {"CLOSE WAYPOINT", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"TYPE", DataTypes::int32, 4},
    {"MAGVAR", DataTypes::float32, 4},
    {"N_ROUTES", DataTypes::int32, 4},
    {"ICAO", DataTypes::string8, 8},
    {"REGION", DataTypes::string8, 8},
    {"IS_TERMINAL_WPT", DataTypes::int32, 4},

    /* Waypoint child: Route */
    {"OPEN ROUTE", DataTypes::invalid, 0},
    {"CLOSE ROUTE", DataTypes::invalid, 0},
    {"NAME", DataTypes::string32, 32},
    {"TYPE", DataTypes::int32, 4},
    {"NEXT_ICAO", DataTypes::string8, 8},
    {"NEXT_REGION", DataTypes::string8, 8},
    {"NEXT_TYPE", DataTypes::int32, 4},
    {"NEXT_LATITUDE", DataTypes::float64, 8},
    {"NEXT_LONGITUDE", DataTypes::float64, 8},
    {"NEXT_ALTITUDE", DataTypes::float32, 4},
    {"PREV_ICAO", DataTypes::string8, 8},
    {"PREV_REGION", DataTypes::string8, 8},
    {"PREV_TYPE", DataTypes::int32, 4},
    {"PREV_LATITUDE", DataTypes::float64, 8},
    {"PREV_LONGITUDE", DataTypes::float64, 8},
    {"PREV_ALTITUDE", DataTypes::float32, 4},

    /* NDB */
    {"OPEN NDB", DataTypes::invalid, 0},
    {"CLOSE NDB", DataTypes::invalid, 0},
    {"LATITUDE", DataTypes::float64, 8},
    {"LONGITUDE", DataTypes::float64, 8},
    {"ALTITUDE", DataTypes::float64, 8},
    {"FREQUENCY", DataTypes::int32, 4},
    {"TYPE", DataTypes::int32, 4},
    {"RANGE", DataTypes::float32, 4},
    {"MAGVAR", DataTypes::float32, 4},
    {"IS_TERMINAL_NDB", DataTypes::int32, 4},
    {"NAME", DataTypes::string64, 64},
    {"BFO_REQUIRED", DataTypes::int32, 4},

    /* VOR */
    {"OPEN VOR", DataTypes::invalid, 0},
    {"CLOSE VOR", DataTypes::invalid, 0},
    {"VOR_LATITUDE", DataTypes::float64, 8},
    {"VOR_LONGITUDE", DataTypes::float64, 8},
    {"VOR_ALTITUDE", DataTypes::float64, 8},
    {"DME_LATITUDE", DataTypes::float64, 8},
    {"DME_LONGITUDE", DataTypes::float64, 8},
    {"DME_ALTITUDE", DataTypes::float64, 8},
    {"GS_LATITUDE", DataTypes::float64, 8},
    {"GS_LONGITUDE", DataTypes::float64, 8},
    {"GS_ALTITUDE", DataTypes::float64, 8},
    {"TACAN_LATITUDE", DataTypes::float64, 8},
    {"TACAN_LONGITUDE", DataTypes::float64, 8},
    {"TACAN_ALTITUDE", DataTypes::float64, 8},
    {"IS_NAV", DataTypes::int32, 4},
    {"IS_DME", DataTypes::int32, 4},
    {"IS_TACAN", DataTypes::int32, 4},
    {"HAS_GLIDE_SLOPE", DataTypes::int32, 4},
    {"DME_AT_NAV", DataTypes::int32, 4},
    {"DME_AT_GLIDE_SLOPE", DataTypes::int32, 4},
    {"HAS_BACK_COURSE", DataTypes::int32, 4},
    {"FREQUENCY", DataTypes::int32, 4},
    {"TYPE", DataTypes::int32, 4},
    {"NAV_RANGE", DataTypes::float32, 4},
    {"MAGVAR", DataTypes::float32, 4},
    {"LOCALIZER", DataTypes::float32, 4},
    {"LOCALIZER_WIDTH", DataTypes::float32, 4},
    {"GLIDE_SLOPE", DataTypes::float32, 4},
    {"NAME", DataTypes::string64, 64},
    {"DME_BIAS", DataTypes::float32, 4},
    {"LS_CATEGORY", DataTypes::int32, 4},
    {"IS_TRUE_REFERENCED", DataTypes::int32, 4},
}};


template <std::size_t MaxLength>
struct FacilityDefinition {
    std::array<FacilityField, MaxLength> fields;
    std::size_t fieldCount;

    constexpr FacilityDefinition() = default;

    constexpr FacilityDefinition push(FacilityField field) const {
        FacilityDefinition newDef{ *this };

        newDef.fields[newDef.fieldCount++] = field; // This will fail to compile if fieldCount >= MaxLength

        return newDef;
    }

};


// Forward declarations
template<std::size_t MaxLength> struct AirportBuilder;
template<std::size_t MaxLength> struct RunwayBuilder;
template<std::size_t MaxLength> struct StartBuilder;
template<std::size_t MaxLength> struct FrequencyBuilder;
template<std::size_t MaxLength> struct HelipadBuilder;
template<std::size_t MaxLength> struct ApproachBuilder;
template<std::size_t MaxLength> struct DepartureBuilder;
template<std::size_t MaxLength> struct ArrivalBuilder;
template<std::size_t MaxLength> struct TaxiParkingBuilder;
template<std::size_t MaxLength> struct TaxiPathBuilder;
template<std::size_t MaxLength> struct TaxiPointBuilder;
template<std::size_t MaxLength> struct TaxiNameBuilder;
template<std::size_t MaxLength> struct JetwayBuilder;
template<std::size_t MaxLength> struct VDGSBuilder;
template<std::size_t MaxLength> struct HoldingPatternBuilder;
template<std::size_t MaxLength> struct PavementBuilder;
template<std::size_t MaxLength> struct ApproachLightsBuilder;
template<std::size_t MaxLength> struct VasiBuilder;
template<std::size_t MaxLength> struct AirlineBuilder;
template<std::size_t MaxLength> struct VORBuilder;
template<std::size_t MaxLength> struct NDBBuilder;


template <std::size_t MaxLength>
struct Builder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit Builder(FacilityDefinition<MaxLength> def = {}) : definition{ def } {}

    constexpr AirportBuilder<MaxLength> airport() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportOpen) };
    }

    constexpr VORBuilder<MaxLength> vor() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorOpen) };
    }
};


template <std::size_t MaxLength>
struct AirlineBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit AirlineBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr TaxiParkingBuilder<MaxLength> end() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::airlineClose) };
    }

    // Field setters
    constexpr AirlineBuilder<MaxLength> name() const {
        return AirlineBuilder<MaxLength>{ definition.push(FacilityField::airlineName) };
    }
};


} // namespace SimConnect::Facilities