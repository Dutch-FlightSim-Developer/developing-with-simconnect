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

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include "output_format.hpp"


template <class data_type>
class Data {
public:

    std::string typeName() const {
        return static_cast<data_type*>(this)->typeName();
	}


    std::string keyName() const {
        return static_cast<data_type*>(this)->keyName();
	}


    /**
     * Return the field names of this Data record.
     */
    std::vector<std::string> fieldNames() {
        return static_cast<data_type*>(this)->fieldNames();
    }


    const std::string& key() const {
        return static_cast<data_type*>(this)->key();
    }


    std::string field(std::string name) const {
		return static_cast<const data_type*>(this)->field(name);
    }


    std::string formattedField(std::string name) {
		return static_cast<data_type*>(this)->formattedField(name);
    }

};