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

#include <filesystem>
#include <optional>
#include <string>
#include <map>
#include <iostream>
#include <fstream>


namespace SimConnect::AI {


inline static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) -> char {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}


inline static std::string trim(std::string s, bool stripQuotes = false) {
    auto notSpace = stripQuotes
        ? [](unsigned char ch) { return !std::isspace(ch) && (ch != '"') && (ch != '\''); }
        : [](unsigned char ch) { return !std::isspace(ch); };

    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());

    return s;
}


/**
 * Light-weight INI file representation.
 * - Sections map to key/value dictionaries
 * - Keys are stored case-insensitive (normalized to lowercase)
 */
class IniFile {
public:
    using Section = std::map<std::string, std::string>; // key(lowercase) -> value


private:
    std::map<std::string, Section> sections_;


public:
    IniFile() = default;


    /**
     * Returns the sections in the INI file.
     * 
     * @returns The sections in the INI file.
     */
    const std::map<std::string, Section>& sections() const noexcept {
        return sections_;
    }


    /**
     * Loads an INI file from the specified path.
     * 
     * @param path The path to the INI file.
     */
    void load(const std::filesystem::path& path) {
        std::ifstream in(path);

        std::string currentSection;

        for (std::string line{}; std::getline(in, line); ) {
            line = trim(line);
            auto endPos = line.size();

            // Remove comments starting with ; or //
            const auto posSemi = line.find(';');
            if (posSemi != std::string::npos) {
                endPos = posSemi;
            }

            const auto posDbl = line.find("//");
            if (posDbl != std::string::npos && posDbl < endPos) {
                endPos = posDbl;
            }
            line = trim(line.substr(0, endPos));

            if (line.empty()) {
                continue;
            }

            // check for a section header
            if (line.front() == '[' && line.back() == ']') {
                currentSection = toLower(trim(line.substr(1, line.size() - 2), true));

                if (!sections_.contains(currentSection)) {
                    sections_.emplace(currentSection, Section{});
                }
                continue;
            }
            // Must be a key-value line
            auto eq = line.find('=');

            if (eq == std::string::npos) {
                continue;
            }

            std::string key = toLower(trim(line.substr(0, eq)));
            std::string value = trim(line.substr(eq + 1), true);

            sections_[currentSection][key] = value;
        }
    }

    
    /**
     * Gets a value from the INI file.
     * 
     * @param section The section name.
     * @param key The key name.
     * @return The optional value, or std::nullopt if not found.
     */
    std::optional<std::string> get(std::string section, std::string key) const
    {
        if (!sections_.contains(section)) { return std::nullopt; }

        const auto& sec = sections_.at(section);
        auto it = sec.find(toLower(key));

        return (it != sec.end()) ? std::optional<std::string>{it->second} : std::nullopt;
    }

};

} // namespace SimConnect::AI
