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
 #include <simconnect/requests/facilities/facility_definition.hpp>
 #include <simconnect/requests/facilities/facility_definition_builder.hpp>

 #include <simconnect/requests/facilities/taxi_parking.hpp>
 #include <simconnect/requests/facilities/frequency.hpp>


 #include <cstdint>

 #include <map>
 #include <array>
 #include <vector>
 #include <string_view>


namespace SimConnect::Facilities {

#pragma pack(push, 1)

/**
 * Represents the data structure for an Airport facility as returned by SimConnect.
 */
class AirportData {
    std::int8_t isClosed_;                          // IS_CLOSED
    std::array<char, ICAOLength> icao_;             // ICAO
    std::array<char, RegionLength> region_;         // REGION
    std::array<char, CountryLength> country_;       // COUNTRY
    std::array<char, CityStateLength> cityState_;   // CITY_STATE
    std::array<char, NameLength> name_;             // NAME
    std::array<char, Name64Length> name64_;         // NAME64

    double latitude_;                               // LATITUDE
    double longitude_;                              // LONGITUDE
    double altitude_;                               // ALTITUDE
    float magVar_;                                  // MAGVAR (Magnetic Variation)

    double towerLatitude_;                          // TOWER_LATITUDE
    double towerLongitude_;                         // TOWER_LONGITUDE
    double towerAltitude_;                          // TOWER_ALTITUDE

    float transitionAltitude_;                      // TRANSITION_ALTITUDE
    float transitionLevel_;                         // TRANSITION_LEVEL

    int32_t nRunways_;                              // N_RUNWAYS
    int32_t nStarts_;                               // N_STARTS
    int32_t nFrequencies_;                          // N_FREQUENCIES
    int32_t nHelipads_;                             // N_HELIPADS
    int32_t nApproaches_;                           // N_APPROACHES
    int32_t nDepartures_;                           // N_DEPARTURES
    int32_t nArrivals_;                             // N_ARRIVALS
    int32_t nTaxiPoints_;                           // N_TAXI_POINTS
    int32_t nTaxiParkings_;                         // N_TAXI_PARKINGS
    int32_t nTaxiPaths_;                            // N_TAXI_PATHS
    int32_t nTaxiNames_;                            // N_TAXI_NAMES
    int32_t nJetways_;                              // N_JETWAYS
    int32_t nVDGS_;                                 // N_VDGS
    int32_t nHoldingPatterns_;                      // N_HOLDING_PATTERNS

public:
    inline static bool isAirportData(const Messages::FacilityDataMsg& msg) {
        return msg.Type == FacilityDataTypes::airport;
    }
    inline static const AirportData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const AirportData*>(&msg.Data);
    }

    /** Returns whether the airport is closed. */
    constexpr bool isClosed() const noexcept { return isClosed_ != 0; }

    /** Returns the ICAO identifier of the airport. */
    constexpr std::string_view icao() const noexcept { return SimConnect::toString(icao_); }
    /** Returns the region code of the airport. */
    constexpr std::string_view region() const noexcept { return SimConnect::toString(region_); }
    /** Returns the country of the airport. */
    constexpr std::string_view country() const noexcept { return SimConnect::toString(country_); }
    /** Returns the city/state of the airport. */
    constexpr std::string_view cityState() const noexcept { return SimConnect::toString(cityState_); }
    /** Returns the name of the airport. */
    constexpr std::string_view name() const noexcept { return SimConnect::toString(name_); }
    /** Returns the 64-character name of the airport. */
    constexpr std::string_view name64() const noexcept { return SimConnect::toString(name64_); }

    /** Returns the latitude of the airport in degrees. */
    constexpr double latitude() const noexcept { return latitude_; }
    /** Returns the longitude of the airport in degrees. */
    constexpr double longitude() const noexcept { return longitude_; }
    /** Returns the altitude of the airport in feet. */
    constexpr double altitude() const noexcept { return altitude_; }
    /** Returns the magnetic variation of the airport in degrees. */
    constexpr float magVar() const noexcept { return magVar_; }

    /** Returns the latitude of the airport tower in degrees. */
    constexpr double towerLatitude() const noexcept { return towerLatitude_; }
    /** Returns the longitude of the airport tower in degrees. */
    constexpr double towerLongitude() const noexcept { return towerLongitude_; }
    /** Returns the altitude of the airport tower in feet. */
    constexpr double towerAltitude() const noexcept { return towerAltitude_; }

    /** Returns the transition altitude in feet. */
    constexpr float transitionAltitude() const noexcept { return transitionAltitude_; }
    /** Returns the transition level in flight levels. */
    constexpr float transitionLevel() const noexcept { return transitionLevel_; }

    /** Returns the number of runways at the airport. */
    constexpr int32_t nRunways() const noexcept { return nRunways_; }
    /** Returns the number of starting positions at the airport. */
    constexpr int32_t nStarts() const noexcept { return nStarts_; }
    /** Returns the number of frequencies at the airport. */
    constexpr int32_t nFrequencies() const noexcept { return nFrequencies_; }
    /** Returns the number of helipads at the airport. */
    constexpr int32_t nHelipads() const noexcept { return nHelipads_; }
    /** Returns the number of approach procedures at the airport. */
    constexpr int32_t nApproaches() const noexcept { return nApproaches_; }
    /** Returns the number of departure procedures at the airport. */
    constexpr int32_t nDepartures() const noexcept { return nDepartures_; }
    /** Returns the number of arrival procedures at the airport. */
    constexpr int32_t nArrivals() const noexcept { return nArrivals_; }
    /** Returns the number of taxi points at the airport. */
    constexpr int32_t nTaxiPoints() const noexcept { return nTaxiPoints_; }
    /** Returns the number of taxi parkings at the airport. */
    constexpr int32_t nTaxiParkings() const noexcept { return nTaxiParkings_; }
    /** Returns the number of taxi paths at the airport. */
    constexpr int32_t nTaxiPaths() const noexcept { return nTaxiPaths_; }
    /** Returns the number of taxi names at the airport. */
    constexpr int32_t nTaxiNames() const noexcept { return nTaxiNames_; }
    /** Returns the number of jetways at the airport. */
    constexpr int32_t nJetways() const noexcept { return nJetways_; }
    /** Returns the number of Visual Docking Guidance Systems at the airport. */
    constexpr int32_t nVDGS() const noexcept { return nVDGS_; }
    /** Returns the number of holding patterns at the airport. */
    constexpr int32_t nHoldingPatterns() const noexcept { return nHoldingPatterns_; }
};

#pragma pack(pop)


/**
 * Key struct for parking names that uses the actual field values.
 * This is much more efficient than parsing formatted strings.
 */
struct ParkingKey {
    ParkingName name;
    int32_t number;
    ParkingName suffix;

    constexpr ParkingKey(ParkingName n, int32_t num, ParkingName suf) 
        : name(n), number(num), suffix(suf) {}

    constexpr bool operator<(const ParkingKey& other) const noexcept {
        // Compare name first
        if (name != other.name) {
            return static_cast<int32_t>(name) < static_cast<int32_t>(other.name);
        }
        // Then compare number
        if (number != other.number) {
            return number < other.number;
        }
        // Finally compare suffix
        return static_cast<int32_t>(suffix) < static_cast<int32_t>(other.suffix);
    }

    constexpr bool operator==(const ParkingKey& other) const noexcept {
        return name == other.name && number == other.number && suffix == other.suffix;
    }
};


/**
 * A version of the AirportData structure that includes (optionally) child data.
 */
struct AirportFacility {
    AirportData data;

    // inline bool haveRunways() const noexcept { return !runways.empty(); }
    // std::vector<RunwayFacility> runways;
    // inline bool haveStarts() const noexcept { return !starts.empty(); }
    // std::vector<StartFacility> starts;
    inline bool haveFrequencies() const noexcept { return !frequencies.empty(); }
    std::vector<FrequencyData> frequencies;
    // inline bool haveHelipads() const noexcept { return !helipads.empty(); }
    // std::vector<HelipadFacility> helipads;
    // inline bool haveApproaches() const noexcept { return !approaches.empty(); }
    // std::vector<ApproachFacility> approaches;
    // inline bool haveDepartures() const noexcept { return !departures.empty(); }
    // std::vector<DepartureFacility> departures;
    // inline bool haveArrivals() const noexcept { return !arrivals.empty(); }
    // std::vector<ArrivalFacility> arrivals;
    inline bool haveTaxiParkings() const noexcept { return !taxiParkings.empty(); }
    std::map<ParkingKey, TaxiParkingFacility> taxiParkings;
    // inline bool haveTaxiPaths() const noexcept { return !taxiPaths.empty(); }
    // std::vector<TaxiPathFacility> taxiPaths;
    // inline bool haveTaxiPoints() const noexcept { return !taxiPoints.empty(); }
    // std::vector<TaxiPointFacility> taxiPoints;
    // inline bool haveTaxiNames() const noexcept { return !taxiNames.empty(); }
    // std::vector<TaxiNameFacility> taxiNames;
    // inline bool haveJetways() const noexcept { return !jetways.empty(); }
    // std::vector<JetwayFacility> jetways;
    // inline bool haveVDGS() const noexcept { return !vdgs.empty(); }
    // std::vector<VDGSFacility> vdgs;
    // inline bool haveHoldingPatterns() const noexcept { return !holdingPatterns.empty(); }
    // std::vector<HoldingPatternFacility> holdingPatterns;
};

/**
 * Builder for constructing Airport facility definitions.
 */
template <std::size_t MaxLength>
struct AirportBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit AirportBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr Builder<MaxLength> end() const {
        return Builder<MaxLength>{ definition.push(FacilityField::airportClose) };
    }

    // Children builders
    constexpr RunwayBuilder<MaxLength> runway() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayOpen) };
    }
    constexpr StartBuilder<MaxLength> start() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startOpen) };
    }
    constexpr FrequencyBuilder<MaxLength> frequency() const {
        return FrequencyBuilder<MaxLength>{ definition.push(FacilityField::frequencyOpen) };
    }
    constexpr HelipadBuilder<MaxLength> helipad() const {
        return HelipadBuilder<MaxLength>{ definition.push(FacilityField::helipadOpen) };
    }
    constexpr ApproachBuilder<MaxLength> approach() const {
        return ApproachBuilder<MaxLength>{ definition.push(FacilityField::approachOpen) };
    }
    constexpr DepartureBuilder<MaxLength> departure() const {
        return DepartureBuilder<MaxLength>{ definition.push(FacilityField::departureOpen) };
    }
    constexpr ArrivalBuilder<MaxLength> arrival() const {
        return ArrivalBuilder<MaxLength>{ definition.push(FacilityField::arrivalOpen) };
    }
    constexpr TaxiParkingBuilder<MaxLength> taxiParking() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingOpen) };
    }
    constexpr TaxiPathBuilder<MaxLength> taxiPath() const {
        return TaxiPathBuilder<MaxLength>{ definition.push(FacilityField::taxiPathOpen) };
    }
    constexpr TaxiPointBuilder<MaxLength> taxiPoint() const {
        return TaxiPointBuilder<MaxLength>{ definition.push(FacilityField::taxiPointOpen) };
    }
    constexpr TaxiNameBuilder<MaxLength> taxiName() const {
        return TaxiNameBuilder<MaxLength>{ definition.push(FacilityField::taxiNameOpen) };
    }
    constexpr JetwayBuilder<MaxLength> jetway() const {
        return JetwayBuilder<MaxLength>{ definition.push(FacilityField::jetwayOpen) };
    }
    constexpr VDGSBuilder<MaxLength> vdgs() const {
        return VDGSBuilder<MaxLength>{ definition.push(FacilityField::vdgsOpen) };
    }
    constexpr HoldingPatternBuilder<MaxLength> holdingPattern() const {
        return HoldingPatternBuilder<MaxLength>{ definition.push(FacilityField::holdingPatternOpen) };
    }

    // Field setters
    constexpr AirportBuilder<MaxLength> latitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportLatitude) };
    }
    constexpr AirportBuilder<MaxLength> longitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportLongitude) };
    }
    constexpr AirportBuilder<MaxLength> altitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportAltitude) };
    }
    constexpr AirportBuilder<MaxLength> magvar() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportMagvar) };
    }
    constexpr AirportBuilder<MaxLength> name() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportName) };
    }
    constexpr AirportBuilder<MaxLength> name64() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportName64) };
    }
    constexpr AirportBuilder<MaxLength> icao() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportICAO) };
    }
    constexpr AirportBuilder<MaxLength> region() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportRegion) };
    }
    constexpr AirportBuilder<MaxLength> towerLatitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTowerLatitude) };
    }
    constexpr AirportBuilder<MaxLength> towerLongitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTowerLongitude) };
    }
    constexpr AirportBuilder<MaxLength> towerAltitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTowerAltitude) };
    }
    constexpr AirportBuilder<MaxLength> transitionAltitude() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTransitionAltitude) };
    }
    constexpr AirportBuilder<MaxLength> transitionLevel() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTransitionLevel) };
    }
    constexpr AirportBuilder<MaxLength> isClosed() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportIsClosed) };
    }
    constexpr AirportBuilder<MaxLength> country() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportCountry) };
    }
    constexpr AirportBuilder<MaxLength> cityState() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportCityState) };
    }
    constexpr AirportBuilder<MaxLength> runways() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportRunways) };
    }
    constexpr AirportBuilder<MaxLength> starts() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportStarts) };
    }
    constexpr AirportBuilder<MaxLength> frequencies() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportFrequencies) };
    }
    constexpr AirportBuilder<MaxLength> helipads() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportHelipads) };
    }
    constexpr AirportBuilder<MaxLength> approaches() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportApproaches) };
    }
    constexpr AirportBuilder<MaxLength> departures() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportDepartures) };
    }
    constexpr AirportBuilder<MaxLength> arrivals() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportArrivals) };
    }
    constexpr AirportBuilder<MaxLength> taxiPoints() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTaxiPoints) };
    }
    constexpr AirportBuilder<MaxLength> taxiParkings() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTaxiParkings) };
    }
    constexpr AirportBuilder<MaxLength> taxiPaths() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTaxiPaths) };
    }
    constexpr AirportBuilder<MaxLength> taxiNames() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportTaxiNames) };
    }
    constexpr AirportBuilder<MaxLength> jetways() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportJetways) };
    }
    constexpr AirportBuilder<MaxLength> vdgsCount() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportVDGS) };
    }
    constexpr AirportBuilder<MaxLength> holdingPatterns() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::airportHoldingPatterns) };
    }

    constexpr AirportBuilder<MaxLength> allFields() const {
        return AirportBuilder<MaxLength>{
            definition
                .push(FacilityField::airportIsClosed)
                .push(FacilityField::airportICAO)
                .push(FacilityField::airportRegion)
                .push(FacilityField::airportCountry)
                .push(FacilityField::airportCityState)
                .push(FacilityField::airportName)
                .push(FacilityField::airportName64)
                .push(FacilityField::airportLatitude)
                .push(FacilityField::airportLongitude)
                .push(FacilityField::airportAltitude)
                .push(FacilityField::airportMagvar)
                .push(FacilityField::airportTowerLatitude)
                .push(FacilityField::airportTowerLongitude)
                .push(FacilityField::airportTowerAltitude)
                .push(FacilityField::airportTransitionAltitude)
                .push(FacilityField::airportTransitionLevel)
                .push(FacilityField::airportRunways)
                .push(FacilityField::airportStarts)
                .push(FacilityField::airportFrequencies)
                .push(FacilityField::airportHelipads)
                .push(FacilityField::airportApproaches)
                .push(FacilityField::airportDepartures)
                .push(FacilityField::airportArrivals)
                .push(FacilityField::airportTaxiPoints)
                .push(FacilityField::airportTaxiParkings)
                .push(FacilityField::airportTaxiPaths)
                .push(FacilityField::airportTaxiNames)
                .push(FacilityField::airportJetways)
                .push(FacilityField::airportVDGS)
                .push(FacilityField::airportHoldingPatterns)
        };
    }
};

} // namespace SimConnect::Facilities