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

#pragma once

#include <string>
#include <map>


#include "data.hpp"


/**
 * Represents an airport from Microsoft Flight Simulator with all relevant data
 */
class Airport : public Data<Airport>
{
    std::string name_{ "" };          // Name of the airport (up to 32 characters)
    std::string longName_{ "" };      // Long name of the airport (up to 64 characters)
    std::string icao_{ "" };          // ICAO identifier (up to 8 characters)
    std::string region_{ "" };        // Region code (up to 2 characters)
    double latitude_{ 0.0 };          // Latitude in degrees
    double longitude_{ 0.0 };         // Longitude in degrees
    double altitude_{ 0.0 };          // Altitude in meters
	float magVar_{ 0.0f };            // Magnetic variation in degrees
	double towerLatitude_{ 0.0 };     // Tower latitude in degrees
	double towerLongitude_{ 0.0 };    // Tower longitude in degrees
	double towerAltitude_{ 0.0 };     // Tower altitude in meters

    int32_t runwayCount_{ 0 };        // Number of runways
	int32_t startCount_{ 0 };         // Number of start positions
	int32_t frequencyCount_{ 0 };     // Number of frequencies
	int32_t helipadCount_{ 0 };       // Number of helipads
	int32_t approachCount_{ 0 };      // Number of approach procedures
	int32_t departureCount_{ 0 };     // Number of departure procedures
	int32_t arrivalCount_{ 0 };       // Number of arrival procedures
    int32_t parkingCount_{ 0 };       // Number of parking positions
    int32_t taxiPointCount_{ 0 };     // Number of taxiway points
    int32_t taxiPathCount_{ 0 };      // Number of taxiway paths
	int32_t taxiNameCount_{ 0 };      // Number of taxiway names
	int32_t jetwayCount_{ 0 };        // Number of jetways

public:
    /**
     * Default constructor
     */
    Airport() = default;
    Airport(std::string icao)
        : name_{ icao }, longName_{ icao }, icao_ { icao }
    {
    }
    Airport(std::string icao, double latitude, double longitude, double altitude)
        : name_{ icao }, longName_{ icao }, icao_(icao), latitude_(latitude), longitude_(longitude), altitude_(altitude)
    {
    }
    Airport(std::string icao, std::string region, double latitude, double longitude, double altitude)
        : name_{ icao }, longName_{ icao }, icao_(icao), region_(region), latitude_(latitude), longitude_(longitude), altitude_(altitude)
    {
    }
    Airport(std::string name, std::string longName, std::string icao, std::string region, double latitude, double longitude, double altitude)
        : name_{ name }, longName_{ longName }, icao_(icao), region_(region), latitude_(latitude), longitude_(longitude), altitude_(altitude)
    {
    }

    Airport(const Airport&) = default;
    Airport(Airport&&) = default;
    Airport& operator=(const Airport&) = default;
    Airport& operator=(Airport&&) = default;
    ~Airport() = default;


    std::string typeName() const {
        return "Airport";
	}


    // Getters
    [[nodiscard]] const std::string& getIcao() const noexcept { return icao_; }
    [[nodiscard]] const std::string& getRegion() const noexcept { return region_; }
    [[nodiscard]] double getLatitude() const noexcept { return latitude_; }
    [[nodiscard]] double getLongitude() const noexcept { return longitude_; }
    [[nodiscard]] double getAltitude() const noexcept { return altitude_; }

    // Getters & setters for the other fields
	[[nodiscard]] const std::string& getName() const noexcept { return name_; }
    void setName(const std::string& name) { name_ = name; }
    void setName(std::string&& name) { name_ = std::move(name); }
	[[nodiscard]] const std::string& getLongName() const noexcept { return longName_; }
    void setLongName(const std::string& longName) { longName_ = longName; }
	void setLongName(std::string&& longName) { longName_ = std::move(longName); }

    [[nodiscard]] float getMagVar() const noexcept { return magVar_; }
	void setMagVar(float magVar) { magVar_ = magVar; }
	
    [[nodiscard]] double getTowerLatitude() const noexcept { return towerLatitude_; }
	[[nodiscard]] double getTowerLongitude() const noexcept { return towerLongitude_; }
	[[nodiscard]] double getTowerAltitude() const noexcept { return towerAltitude_; }
	void setTowerLocation(double latitude, double longitude, double altitude) {
        towerLatitude_ = latitude; towerLongitude_ = longitude; towerAltitude_ = altitude;
    }

    [[nodiscard]] int32_t getRunwayCount() const noexcept { return runwayCount_; }
	void setRunwayCount(int32_t runwayCount) { runwayCount_ = runwayCount; }
	[[nodiscard]] int32_t getStartCount() const noexcept { return startCount_; }
	void setStartCount(int32_t startCount) { startCount_ = startCount; }
	[[nodiscard]] int32_t getFrequencyCount() const noexcept { return frequencyCount_; }
	void setFrequencyCount(int32_t frequencyCount) { frequencyCount_ = frequencyCount; }
	[[nodiscard]] int32_t getHelipadCount() const noexcept { return helipadCount_; }
	void setHelipadCount(int32_t helipadCount) { helipadCount_ = helipadCount; }
	[[nodiscard]] int32_t getApproachCount() const noexcept { return approachCount_; }
	void setApproachCount(int32_t approachCount) { approachCount_ = approachCount; }
	[[nodiscard]] int32_t getDepartureCount() const noexcept { return departureCount_; }
	void setDepartureCount(int32_t departureCount) { departureCount_ = departureCount; }
	[[nodiscard]] int32_t getArrivalCount() const noexcept { return arrivalCount_; }
	void setArrivalCount(int32_t arrivalCount) { arrivalCount_ = arrivalCount; }
	[[nodiscard]] int32_t getParkingCount() const noexcept { return parkingCount_; }
	void setParkingCount(int32_t parkingCount) { parkingCount_ = parkingCount; }
	[[nodiscard]] int32_t getTaxiPointCount() const noexcept { return taxiPointCount_; }
	void setTaxiPointCount(int32_t taxiPointCount) { taxiPointCount_ = taxiPointCount; }
	[[nodiscard]] int32_t getTaxiPathCount() const noexcept { return taxiPathCount_; }
	void setTaxiPathCount(int32_t taxiPathCount) { taxiPathCount_ = taxiPathCount; }
	[[nodiscard]] int32_t getTaxiNameCount() const noexcept { return taxiNameCount_; }
	void setTaxiNameCount(int32_t taxiNameCount) { taxiNameCount_ = taxiNameCount; }
	[[nodiscard]] int32_t getJetwayCount() const noexcept { return jetwayCount_; }
	void setJetwayCount(int32_t jetwayCount) { jetwayCount_ = jetwayCount; }


    const std::string& key() const { return icao_; }


    /**
     * Comparison operator for sorting by ICAO code
     */
    [[nodiscard]] bool operator<(const Airport& other) const noexcept {
        return icao_ < other.icao_;
    }

    /**
     * Equality operator
     */
    [[nodiscard]] bool operator==(const Airport& other) const noexcept {
        return icao_ == other.icao_;
    }

    // Implementations of Data interface methods

    constexpr std::string keyName() const {
        return "ICAO";
	}

    [[nodiscard]]
    constexpr std::vector<std::string> fieldNames() const {
        return {"ICAO", "Region", "Latitude", "Longitude", "Altitude"};
	}


    std::string field(std::string name) const {
        if (name == "ICAO") return icao_;
        if (name == "Region") return region_;
        if (name == "Latitude") return std::to_string(latitude_);
        if (name == "Longitude") return std::to_string(longitude_);
        if (name == "Altitude") return std::to_string(altitude_);
        return "";
	}


    std::string formattedField(std::string name) const {
        if (name == "ICAO") return std::format("\"{}\"", icao_);
        if (name == "Region") return std::format("\"{}\"", region_);
        if (name == "Latitude") return std::to_string(latitude_);
        if (name == "Longitude") return std::to_string(longitude_);
        if (name == "Altitude") return std::to_string(altitude_);
        return "";
    }


    inline static bool addToDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID defId) {
        return SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "OPEN AIRPORT"))

			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "MAGVAR"))
            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "NAME"))
            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "NAME64"))

            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "TOWER_LATITUDE"))
            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "TOWER_LONGITUDE"))
            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "TOWER_ALTITUDE"))

			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_RUNWAYS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_STARTS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_FREQUENCIES"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_HELIPADS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_APPROACHES"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_DEPARTURES"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_ARRIVALS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_TAXI_PARKINGS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_TAXI_POINTS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_TAXI_PATHS"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_TAXI_NAMES"))
			&& SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "N_JETWAYS"))

            && SUCCEEDED(SimConnect_AddToFacilityDefinition(hSimConnect, defId, "CLOSE AIRPORT"));
    }

#pragma pack(push, 1) // Ensure no padding bytes are added to the structure
    struct AirportData {
        float magvar;          // Magnetic variation in degrees
		char name[32];        // Name of the airport (up to 32 characters)
        char longName[64];

		double towerLatitude;   // Tower latitude in degrees
		double towerLongitude;  // Tower longitude in degrees
		double towerAltitude;   // Tower altitude in meters

        int32_t runwayCount;    // Number of runways
        int32_t startCount;     // Number of start positions
        int32_t frequencyCount; // Number of frequencies
        int32_t helipadCount;   // Number of helipads
        int32_t approachCount;  // Number of approach procedures
        int32_t departureCount; // Number of departure procedures
        int32_t arrivalCount;   // Number of arrival procedures
        int32_t parkingCount;   // Number of parking positions
        int32_t taxiPointCount; // Number of taxiway points
        int32_t taxiPathCount;  // Number of taxiway paths
        int32_t taxiNameCount;  // Number of taxiway names
		int32_t jetwayCount;    // Number of jetways
    };
#pragma pack(pop) // Restore previous packing alignment

    void copyData(const SIMCONNECT_RECV_FACILITY_DATA* pFacilityData) {
        const auto* pData = reinterpret_cast<const AirportData*>(&pFacilityData->Data);
        magVar_ = pData->magvar;
        name_ = std::string(pData->name, 32);
        longName_ = std::string(pData->longName, 64);
        towerLatitude_ = pData->towerLatitude;
        towerLongitude_ = pData->towerLongitude;
        towerAltitude_ = pData->towerAltitude;
        runwayCount_ = pData->runwayCount;
        startCount_ = pData->startCount;
        frequencyCount_ = pData->frequencyCount;
        helipadCount_ = pData->helipadCount;
        approachCount_ = pData->approachCount;
        departureCount_ = pData->departureCount;
        arrivalCount_ = pData->arrivalCount;
        parkingCount_ = pData->parkingCount;
        taxiPointCount_ = pData->taxiPointCount;
        taxiPathCount_ = pData->taxiPathCount;
        taxiNameCount_ = pData->taxiNameCount;
    }
};