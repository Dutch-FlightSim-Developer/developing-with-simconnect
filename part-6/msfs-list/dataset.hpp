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

#include <map>
#include <string>

#include "data.hpp"


template <class data_type>
class DataSet
{
	std::map<std::string, data_type> collection_;


    void indent(std::ostream& outputStream, int indent) const {
        for (int i = 0; i < indent; ++i) {
            outputStream << "  "; // Two spaces for indentation
        }
    }


public:

	DataSet() = default;
	DataSet(const DataSet&) = default;
	DataSet(DataSet&&) = default;
	DataSet& operator=(const DataSet&) = default;
	DataSet& operator=(DataSet&&) = default;
	~DataSet() = default;


	void add(const data_type& item) {
		collection_[item.key()] = item;
	}
	void add(data_type&& item) {
		collection_[item.key()] = std::move(item);
	}

	bool contains(std::string key) const noexcept {
		return collection_.contains(key);
	}
	const data_type& get(std::string key) const noexcept {
		return collection_.at(key);
	}
	void clear() {
		collection_.clear();
	}
	size_t size() const noexcept {
		return collection_.size();
	}
	const std::map<std::string, data_type>& all() const noexcept {
		return collection_;
	}


    void stream(std::ostream& os, std::string key, OutputFormat format = OutputFormat::Text, int level = 0) const {
        if (!collection_.contains(key)) {
			return; // No item with this key
		}
		const auto& item = collection_.at(key);
        switch (format) {
        case OutputFormat::Text:
        {
            indent(os, level);
            os << item.typeName() << "(";
            bool first{ true };
            for (const auto& field : item.fieldNames()) {
                if (first) {
                    first = false;
                }
                else {
                    os << ", ";
                }
                os << field << ": " << item.formattedField(field);
            }
            os << ")";
        }
            break;
        case OutputFormat::Csv:
        {
            bool first{ true };
            for (const auto& field : item.fieldNames()) {
                if (first) {
                    first = false;
                }
                else {
                    os << ", ";
                }
                os << item.formattedField(field);
            }
        }
            break;
        case OutputFormat::Json:
        {
            bool first = true;
            indent(os, level);  os << "{\n";
            for (const auto& field : item.fieldNames()) {
                if (first) {
                    first = false;
                }
                else {
                    os << ",\n";
                }
                indent(os, level + 1);
                os << "\"" << field << "\": " << item.formattedField(field);
            }
            os << "\n";
            indent(os, level); os << "}";
        }
            break;
        case OutputFormat::Yaml:
        {
            indent(os, level); os << item.typeName() << ":\n";
            for (const auto& field : item.fieldNames()) {
                indent(os, level + 1);
                os << field << ": " << item.formattedField(field) << "\n";
            }
        }
            break;
        }
	}


    /**
     * Stream all data records from a collection using the current format
     * @param os The output stream to write to
     */
    void streamAll(std::ostream& os, OutputFormat format = OutputFormat::Text, int level = 0) {
        switch (format) {
        case OutputFormat::Text:
            {
                for (const auto& entry : collection_) {
				    stream(os, entry.first, format, level);
				    os << "\n";
                }
            }
            break;

        case OutputFormat::Csv:
            {
                if (collection_.empty()) {
                        return; // No items to stream
			    }
                bool firstRow{ true };
                for (const auto& entry : collection_) {
                    if (firstRow) {
                        bool first{ true };
                        for (auto field : entry.second.fieldNames()) {
                            if (first) {
                                first = false;
                            }
                            else {
                                os << ",";
                            }
                            os << field;
                        }
                        os << "\n";
                    }
                    stream(os, entry.first, format, level);
                    os << "\n";
                }
			}
            break;

        case OutputFormat::Json:
            {
                indent(os, level); os << "[";
                bool first = true;
                for (const auto& entry : collection_) {
                    if (first) {
                        first = false;
                    }
                    else {
                        os << ",";
                    }
                    stream(os, entry.first, format, level + 1);
					os << "\n";
                }
                indent(os, level); os << "]\n";
		    }
			break;

        case OutputFormat::Yaml:
            {
                for (const auto& entry : collection_) {
                    stream(os, entry.first, format, level);
                    os << "\n";
                }
            }
			break;
        }
    }


};