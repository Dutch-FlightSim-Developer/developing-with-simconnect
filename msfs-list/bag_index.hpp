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
#include <set>
#include <string>


template <class data_type>
class BagIndex
{
	std::map<std::string, std::set<data_type>> collection_;


public:

	BagIndex() = default;
	BagIndex(const BagIndex&) = default;
	BagIndex(BagIndex&&) = default;
	BagIndex& operator=(const BagIndex&) = default;
	BagIndex& operator=(BagIndex&&) = default;
	~BagIndex() = default;


	void add(std::string key, const data_type& item) {
		collection_[key].insert(item);
	}

	bool contains(std::string key) const noexcept {
		return collection_.contains(key);
	}
	const std::set<data_type>& get(std::string key) const noexcept {
		return collection_.at(key);
	}
	void clear() {
		collection_.clear();
	}
	size_t size() const noexcept {
		return collection_.size();
	}
	const std::map<std::string, std::set<data_type>>& all() const noexcept {
		return collection_;
	}
};