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
#include <set>
#include <vector>
#include <ostream>
#include <stdexcept>

#include "data.hpp"

/**
 * Represents a livery from Microsoft Flight Simulator
 */
class Livery : public Data<Livery>
{
private:
    std::string key_;
    std::string title_;     // Aircraft title
    std::string livery_;    // Livery name


public:
    /**
     * Default constructor
     */
    Livery() = default;

    /**
     * Constructor with parameters
     */
    Livery(std::string title, std::string livery)
        : title_(std::move(title)), livery_(std::move(livery))
    {
		key_ = title_ + "|" + livery_;  // Generate key from title and livery
    }

    Livery(const Livery&) = default;
    Livery(Livery&&) = default;
    Livery& operator=(const Livery&) = default;
    Livery& operator=(Livery&&) = default;
    ~Livery() = default;


    std::string typeName() const {
        return "Livery";
	}


    // Key for this livery, used in collections
	const std::string& key() const { return key_; }

    // Getters
    [[nodiscard]] const std::string& getTitle() const noexcept { return title_; }
    [[nodiscard]] const std::string& getLivery() const noexcept { return livery_; }

    /**
     * Comparison operator for sorting by title first, then livery
     */
    [[nodiscard]] bool operator<(const Livery& other) const noexcept {
        if (title_ != other.title_) {
            return title_ < other.title_;
        }
        return livery_ < other.livery_;
    }

    /**
     * Equality operator
     */
    [[nodiscard]] bool operator==(const Livery& other) const noexcept {
        return title_ == other.title_ && livery_ == other.livery_;
    }

    std::string keyName() const {
        return "Title|Livery";
	}

    [[nodiscard]]
    std::vector<std::string> fieldNames() const {
        return {"Title", "Livery"};
    }

    std::string field(std::string name) const {
        if (name == "Title") {
            return title_;
        }
        else if (name == "Livery") {
            return livery_;
        }
        throw std::invalid_argument("Invalid field name: " + name);
	}

    std::string formattedField(std::string name) const {
        if (name == "Title") {
            return std::format("\"{}\"", title_);
        }
        else if (name == "Livery") {
            return std::format("\"{}\"", livery_);
        }
        throw std::invalid_argument("Invalid field name: " + name);
	}
};
