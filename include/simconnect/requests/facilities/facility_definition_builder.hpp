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
    runwayPrimaryThresholdOpen,
    runwayPrimaryThresholdClose,
    runwayPrimaryBlastpadOpen,
    runwayPrimaryBlastpadClose,
    runwayPrimaryOverrunOpen,
    runwayPrimaryOverrunClose,
    runwayPrimaryApproachLightsOpen,
    runwayPrimaryApproachLightsClose,
    runwayPrimaryLeftVASIOpen,
    runwayPrimaryLeftVASIClose,
    runwayPrimaryRightVASIOpen,
    runwayPrimaryRightVASIClose,
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
    runwaySecondaryThresholdOpen,
    runwaySecondaryThresholdClose,
    runwaySecondaryBlastpadOpen,
    runwaySecondaryBlastpadClose,
    runwaySecondaryOverrunOpen,
    runwaySecondaryOverrunClose,
    runwaySecondaryApproachLightsOpen,
    runwaySecondaryApproachLightsClose,
    runwaySecondaryLeftVASIOpen,
    runwaySecondaryLeftVASIClose,
    runwaySecondaryRightVASIOpen,
    runwaySecondaryRightVASIClose,

    /* Runway child fields: Pavement */
    pavementLength,
    pavementWidth,
    pavementEnable,

    /* Runway child fields: ApproachLights */
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

    /* Runway child fields: VASI */
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
    FieldCount
};

inline constexpr std::size_t FacilityFieldCount = static_cast<std::size_t>(FacilityField::FieldCount);

inline constexpr std::array<std::string_view, FacilityFieldCount> facilityFieldInfos = {{
    /* Airports */
    "OPEN AIRPORT",
    "CLOSE AIRPORT",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "MAGVAR",
    "NAME",
    "NAME64",
    "ICAO",
    "REGION",
    "TOWER_LATITUDE",
    "TOWER_LONGITUDE",
    "TOWER_ALTITUDE",
    "TRANSITION_ALTITUDE",
    "TRANSITION_LEVEL",
    "IS_CLOSED",
    "COUNTRY",
    "CITY_STATE",
    "N_RUNWAYS",
    "N_STARTS",
    "N_FREQUENCIES",
    "N_HELIPADS",
    "N_APPROACHES",
    "N_DEPARTURES",
    "N_ARRIVALS",
    "N_TAXI_POINTS",
    "N_TAXI_PARKINGS",
    "N_TAXI_PATHS",
    "N_TAXI_NAMES",
    "N_JETWAYS",
    "N_VDGS",
    "N_HOLDING_PATTERNS",

    /* Airport child: Runways */
    "OPEN RUNWAY",
    "CLOSE RUNWAY",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "HEADING",
    "LENGTH",
    "WIDTH",
    "PATTERN_ALTITUDE",
    "SLOPE",
    "TRUE_SLOPE",
    "SURFACE",
    "PRIMARY_ILS_ICAO",
    "PRIMARY_ILS_REGION",
    "PRIMARY_ILS_TYPE",
    "PRIMARY_NUMBER",
    "PRIMARY_DESIGNATOR",
    "OPEN PRIMARY_THRESHOLD",
    "CLOSE PRIMARY_THRESHOLD",
    "OPEN PRIMARY_BLASTPAD",
    "CLOSE PRIMARY_BLASTPAD",
    "OPEN PRIMARY_OVERRUN",
    "CLOSE PRIMARY_OVERRUN",
    "OPEN PRIMARY_APPROACH_LIGHTS",
    "CLOSE PRIMARY_APPROACH_LIGHTS",
    "OPEN PRIMARY_LEFT_VASI",
    "CLOSE PRIMARY_LEFT_VASI",
    "OPEN PRIMARY_RIGHT_VASI",
    "CLOSE PRIMARY_RIGHT_VASI",
    "SECONDARY_ILS_ICAO",
    "SECONDARY_ILS_REGION",
    "SECONDARY_ILS_TYPE",
    "SECONDARY_NUMBER",
    "SECONDARY_DESIGNATOR",
    "EDGE_LIGHTS",
    "CENTER_LIGHTS",
    "PRIMARY_CLOSED",
    "SECONDARY_CLOSED",
    "PRIMARY_TAKEOFF",
    "PRIMARY_LANDING",
    "SECONDARY_TAKEOFF",
    "SECONDARY_LANDING",
    "OPEN SECONDARY_THRESHOLD",
    "CLOSE SECONDARY_THRESHOLD",
    "OPEN SECONDARY_BLASTPAD",
    "CLOSE SECONDARY_BLASTPAD",
    "OPEN SECONDARY_OVERRUN",
    "CLOSE SECONDARY_OVERRUN",
    "OPEN SECONDARY_APPROACH_LIGHTS",
    "CLOSE SECONDARY_APPROACH_LIGHTS",
    "OPEN SECONDARY_LEFT_VASI",
    "CLOSE SECONDARY_LEFT_VASI",
    "OPEN SECONDARY_RIGHT_VASI",
    "CLOSE SECONDARY_RIGHT_VASI",

    /* Runway child fields: Pavement */
    "LENGTH",
    "WIDTH",
    "ENABLE",

    /* Runway child fields: ApproachLights */
    "SYSTEM",
    "STROBE_COUNT",
    "HAS_END_LIGHTS",
    "HAS_REIL_LIGHTS",
    "HAS_TOUCHDOWN_LIGHTS",
    "ON_GROUND",
    "ENABLE",
    "OFFSET",
    "SPACING",
    "SLOPE",

    /* Runway child fields: VASI */
    "TYPE",
    "BIAS_X",
    "BIAS_Z",
    "SPACING",
    "ANGLE",

    /* Airport child: Start */
    "OPEN START",
    "CLOSE START",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "HEADING",
    "NUMBER",
    "DESIGNATOR",
    "TYPE",

    /* Airport child: Frequency */
    "OPEN FREQUENCY",
    "CLOSE FREQUENCY",
    "TYPE",
    "FREQUENCY",
    "NAME",

    /* Airport child: Helipad */
    "OPEN HELIPAD",
    "CLOSE HELIPAD",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "HEADING",
    "LENGTH",
    "WIDTH",
    "SURFACE",
    "TYPE",
    "TOUCH_DOWN_LENGTH",
    "FATO_LENGTH",
    "FATO_WIDTH",

    /* Airport child: Approach */
    "OPEN APPROACH",
    "CLOSE APPROACH",
    "TYPE",
    "SUFFIX",
    "RUNWAY_NUMBER",
    "RUNWAY_DESIGNATOR",
    "FAF_ICAO",
    "FAF_REGION",
    "FAF_HEADING",
    "FAF_ALTITUDE",
    "FAF_TYPE",
    "MISSED_ALTITUDE",
    "HAS_LNAV",
    "HAS_LNAVVNAV",
    "HAS_LP",
    "HAS_LPV",
    "IS_RNPAR",
    "IS_RNPAR_MISSED",
    "N_TRANSITIONS",
    "N_FINAL_APPROACH_LEGS",
    "N_MISSED_APPROACH_LEGS",

    /* Approach child: ApproachTransition */
    "OPEN APPROACH_TRANSITION",
    "CLOSE APPROACH_TRANSITION",
    "TYPE",
    "IAF_ICAO",
    "IAF_REGION",
    "IAF_TYPE",
    "IAF_ALTITUDE",
    "DME_ARC_ICAO",
    "DME_ARC_REGION",
    "DME_ARC_TYPE",
    "DME_ARC_RADIAL",
    "DME_ARC_DISTANCE",
    "NAME",
    "N_APPROACH_LEGS",

    /* Approach/Transition child: ApproachLeg / FinalApproachLeg / MissedApproachLeg */
    "OPEN APPROACH_LEG",
    "CLOSE APPROACH_LEG",
    "TYPE",
    "FIX_ICAO",
    "FIX_REGION",
    "FIX_TYPE",
    "FIX_LATITUDE",
    "FIX_LONGITUDE",
    "FIX_ALTITUDE",
    "FLY_OVER",
    "DISTANCE_MINUTE",
    "TRUE_DEGREE",
    "TURN_DIRECTION",
    "ORIGIN_ICAO",
    "ORIGIN_REGION",
    "ORIGIN_TYPE",
    "ORIGIN_LATITUDE",
    "ORIGIN_LONGITUDE",
    "ORIGIN_ALTITUDE",
    "THETA",
    "RHO",
    "COURSE",
    "ROUTE_DISTANCE",
    "APPROACH_ALT_DESC",
    "ALTITUDE1",
    "ALTITUDE2",
    "SPEED_LIMIT",
    "VERTICAL_ANGLE",
    "ARC_CENTER_FIX_ICAO",
    "ARC_CENTER_FIX_REGION",
    "ARC_CENTER_FIX_TYPE",
    "ARC_CENTER_FIX_LATITUDE",
    "ARC_CENTER_FIX_LONGITUDE",
    "ARC_CENTER_FIX_ALTITUDE",
    "RADIUS",
    "IS_IAF",
    "IS_IF",
    "IS_FAF",
    "IS_MAP",
    "REQUIRED_NAVIGATION_PERFORMANCE",
    "APPROACH_SPEED_DESC",

    /* Approach child: FinalApproachLeg (uses APPROACH_LEG fields) */
    "OPEN FINAL_APPROACH_LEG",
    "CLOSE FINAL_APPROACH_LEG",

    /* Approach child: MissedApproachLeg (uses APPROACH_LEG fields) */
    "OPEN MISSED_APPROACH_LEG",
    "CLOSE MISSED_APPROACH_LEG",

    /* Airport child: Departure */
    "OPEN DEPARTURE",
    "CLOSE DEPARTURE",
    "NAME",
    "IS_RNPAR",
    "N_RUNWAY_TRANSITIONS",
    "N_ENROUTE_TRANSITIONS",
    "N_APPROACH_LEGS",

    /* Departure/Arrival child: RunwayTransition */
    "OPEN RUNWAY_TRANSITION",
    "CLOSE RUNWAY_TRANSITION",
    "RUNWAY_NUMBER",
    "RUNWAY_DESIGNATOR",
    "N_APPROACH_LEGS",

    /* Departure/Arrival child: EnrouteTransition */
    "OPEN ENROUTE_TRANSITION",
    "CLOSE ENROUTE_TRANSITION",
    "NAME",
    "N_APPROACH_LEGS",

    /* Airport child: Arrival */
    "OPEN ARRIVAL",
    "CLOSE ARRIVAL",
    "NAME",
    "IS_RNPAR",
    "N_RUNWAY_TRANSITIONS",
    "N_ENROUTE_TRANSITIONS",
    "N_APPROACH_LEGS",

    /* Airport child: TaxiParking */
    "OPEN TAXI_PARKING",
    "CLOSE TAXI_PARKING",
    "TYPE",
    "TAXI_POINT_TYPE",
    "NAME",
    "SUFFIX",
    "NUMBER",
    "ORIENTATION",
    "HEADING",
    "RADIUS",
    "BIAS_X",
    "BIAS_Z",
    "N_AIRLINES",

    /* TaxiParking child: Airline */
    "OPEN AIRLINE",
    "CLOSE AIRLINE",
    "NAME",

    /* Airport child: TaxiPoint */
    "OPEN TAXI_POINT",
    "CLOSE TAXI_POINT",
    "TYPE",
    "ORIENTATION",
    "BIAS_X",
    "BIAS_Z",

    /* Airport child: TaxiPath */
    "OPEN TAXI_PATH",
    "CLOSE TAXI_PATH",
    "TYPE",
    "WIDTH",
    "LEFT_HALF_WIDTH",
    "RIGHT_HALF_WIDTH",
    "WEIGHT",
    "RUNWAY_NUMBER",
    "RUNWAY_DESIGNATOR",
    "LEFT_EDGE",
    "LEFT_EDGE_LIGHTED",
    "RIGHT_EDGE",
    "RIGHT_EDGE_LIGHTED",
    "CENTER_LINE",
    "CENTER_LINE_LIGHTED",
    "START",
    "END",
    "NAME_INDEX",

    /* Airport child: TaxiName */
    "OPEN TAXI_NAME",
    "CLOSE TAXI_NAME",
    "NAME",

    /* Airport child: Jetway */
    "OPEN JETWAY",
    "CLOSE JETWAY",
    "PARKING_GATE",
    "PARKING_SUFFIX",
    "PARKING_SPOT",

    /* Airport child: VDGS */
    "OPEN VDGS",
    "CLOSE VDGS",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "PARKING_NUMBER",
    "PARKING_GATE",
    "PARKING_SUFFIX",
    "PARKING_INDEX",

    /* Airport child: HoldingPattern */
    "OPEN HOLDING_PATTERN",
    "CLOSE HOLDING_PATTERN",
    "NAME",
    "FIX_ICAO",
    "FIX_REGION",
    "FIX_TYPE",
    "INBOUND_HOLDING_COURSE",
    "TURN_RIGHT",
    "LEG_LENGTH",
    "LEG_TIME",
    "MIN_ALTITUDE",
    "MAX_ALTITUDE",
    "HOLD_SPEED",
    "REQUIRED_NAVIGATION_PERFORMANCE",
    "ARC_RADIUS",

    /* Waypoints */
    "OPEN WAYPOINT",
    "CLOSE WAYPOINT",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "TYPE",
    "MAGVAR",
    "N_ROUTES",
    "ICAO",
    "REGION",
    "IS_TERMINAL_WPT",

    /* Waypoint child: Route */
    "OPEN ROUTE",
    "CLOSE ROUTE",
    "NAME",
    "TYPE",
    "NEXT_ICAO",
    "NEXT_REGION",
    "NEXT_TYPE",
    "NEXT_LATITUDE",
    "NEXT_LONGITUDE",
    "NEXT_ALTITUDE",
    "PREV_ICAO",
    "PREV_REGION",
    "PREV_TYPE",
    "PREV_LATITUDE",
    "PREV_LONGITUDE",
    "PREV_ALTITUDE",

    /* NDB */
    "OPEN NDB",
    "CLOSE NDB",
    "LATITUDE",
    "LONGITUDE",
    "ALTITUDE",
    "FREQUENCY",
    "TYPE",
    "RANGE",
    "MAGVAR",
    "IS_TERMINAL_NDB",
    "NAME",
    "BFO_REQUIRED",

    /* VOR */
    "OPEN VOR",
    "CLOSE VOR",
    "VOR_LATITUDE",
    "VOR_LONGITUDE",
    "VOR_ALTITUDE",
    "DME_LATITUDE",
    "DME_LONGITUDE",
    "DME_ALTITUDE",
    "GS_LATITUDE",
    "GS_LONGITUDE",
    "GS_ALTITUDE",
    "TACAN_LATITUDE",
    "TACAN_LONGITUDE",
    "TACAN_ALTITUDE",
    "IS_NAV",
    "IS_DME",
    "IS_TACAN",
    "HAS_GLIDE_SLOPE",
    "DME_AT_NAV",
    "DME_AT_GLIDE_SLOPE",
    "HAS_BACK_COURSE",
    "FREQUENCY",
    "TYPE",
    "NAV_RANGE",
    "MAGVAR",
    "LOCALIZER",
    "LOCALIZER_WIDTH",
    "GLIDE_SLOPE",
    "NAME",
    "DME_BIAS",
    "LS_CATEGORY",
    "IS_TRUE_REFERENCED",
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
template<std::size_t MaxLength> struct JetwayBuilder;
template<std::size_t MaxLength> struct VDGSBuilder;
template<std::size_t MaxLength> struct HoldingPatternBuilder;
template<std::size_t MaxLength> struct PavementBuilder;
template<std::size_t MaxLength> struct ApproachLightsBuilder;
template<std::size_t MaxLength> struct VasiBuilder;
template<std::size_t MaxLength> struct AirlineBuilder;
template<std::size_t MaxLength> struct VORBuilder;
template<std::size_t MaxLength> struct NDBBuilder;
template<std::size_t MaxLength> struct WaypointBuilder;
template<std::size_t MaxLength> struct RouteBuilder;


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

    constexpr NDBBuilder<MaxLength> ndb() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbOpen) };
    }

    constexpr WaypointBuilder<MaxLength> waypoint() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointOpen) };
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