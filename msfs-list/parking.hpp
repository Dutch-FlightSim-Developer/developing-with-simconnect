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
#include <format>


#include "data.hpp"

class Parking : public Data<Parking>
{
	std::string name_;          // Name of the parking
	std::string airportIcao_;   // ICAO identifier of the airport this parking belongs to
	uint32_t number_;            // Parking number
	std::string type_;          // Parking type (e.g., "GATE", "RAMP", "HELIPAD")
	std::string gateName_;      // Gate name (if applicable)
	std::string suffix_;        // Suffix (if applicable)
	std::string taxiPointType_; // Taxi point type
	bool reverse_;				// If true, aircraft should be parked facing outwards
	float heading_;             // Heading in degrees
	float radius_;              // Radius in meters
	float biasX_;				// Bias in X direction in meters
	float biasZ_;				// Bias in Z direction in meters
	int32_t airlineCount_;      // Number of airlines associated with this parking


public:
	Parking() = default;
	Parking(const Parking&) = default;
	Parking(Parking&&) = default;

	Parking(std::string airportIcao,
			uint32_t number, std::string type, std::string gateName, std::string suffix,
			std::string taxiPointType,
			bool reverse, float heading, float radius, float biasX, float biasZ,
			int32_t airlineCount)
		: airportIcao_(std::move(airportIcao))
		, number_(number), type_(std::move(type)), gateName_(std::move(gateName)), suffix_(std::move(suffix))
		, taxiPointType_(std::move(taxiPointType))
		, reverse_(reverse), heading_(heading), radius_(radius), biasX_(biasX), biasZ_(biasZ)
		, airlineCount_(airlineCount)
	{
		name_ = gateName_;
		if (!name_.empty() && (number_ != 0)) {
			name_ += " ";
			name_ += std::to_string(number_);
		}
		if (!suffix_.empty()) {
			if (!name_.empty()) name_ += " ";
			name_ += suffix_;
		}
	}

	Parking& operator=(const Parking&) = default;
	Parking& operator=(Parking&&) = default;

	~Parking() = default;

	std::string typeName() const { return "Parking"; }
	std::string keyName() const { return "ParkingName"; }
	std::vector<std::string> fieldNames() {
		return {
			"Name", "AirportICAO",
			"ParkingNumber", "Type", "GateName", "Suffix",
			"TaxiPointType",
			"Reverse", "Heading", "Radius", "BiasX", "BiasZ",
			"AirlineCount"
		};
	}
	const std::string& key() const { return name_; }
	std::string field(std::string name) const {
		if (name == "Name") return name_;
		if (name == "AirportICAO") return airportIcao_;
		if (name == "ParkingNumber") return std::to_string(number_);
		if (name == "Type") return type_;
		if (name == "GateName") return gateName_;
		if (name == "Suffix") return suffix_;
		if (name == "TaxiPointType") return taxiPointType_;
		if (name == "Reverse") return std::to_string(reverse_);
		if (name == "Heading") return std::to_string(heading_);
		if (name == "Radius") return std::to_string(radius_);
		if (name == "BiasX") return std::to_string(biasX_);
		if (name == "BiasZ") return std::to_string(biasZ_);
		if (name == "AirlineCount") return std::to_string(airlineCount_);
		return "";
	}
	std::string formattedField(std::string name) {
		if (name == "Name") return std::format("\"{}\"", name_);
		if (name == "AirportICAO") return std::format("\"{}\"", airportIcao_);
		if (name == "ParkingNumber") return std::to_string(number_);
		if (name == "Type") return std::format("\"{}\"", type_);
		if (name == "GateName") return std::format("\"{}\"", gateName_);
		if (name == "Suffix") return std::format("\"{}\"", suffix_);
		if (name == "TaxiPointType") return std::format("\"{}\"", taxiPointType_);
		if (name == "Reverse") return std::to_string(reverse_);
		if (name == "Heading") return std::to_string(heading_);
		if (name == "Radius") return std::to_string(radius_);
		if (name == "BiasX") return std::to_string(biasX_);
		if (name == "BiasZ") return std::to_string(biasZ_);
		if (name == "AirlineCount") return std::to_string(airlineCount_);
		return "";
	}
};