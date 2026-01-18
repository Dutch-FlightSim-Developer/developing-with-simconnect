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

#include <simconnect/util/uuid.h>

#include <map>
#include <set>
#include <string>
#include <optional>
#include <fstream>
#include <iostream>
#include <filesystem>


namespace SimConnect::AI {


struct SimObjectInfo {
    std::string id;                                     ///< The unique identifier (UUID)of the SimObject.
    std::optional<std::string> tag;                     ///< An optional, but unique, tag of the SimObject.
    SimObjectType type{ SimObjectTypes::aircraft };     ///< The type of the SimObject.
    std::string title;                                  ///< The title of the SimObject.
    std::optional<std::string> livery;                  ///< An optional livery of the SimObject. (if from MSFS 2024)
};


/**
 * Repository for SimObjects.
 */
class SimObjectRepository {
    std::filesystem::path repositoryPath_;                      ///< The path to the SimObject repository.
    std::map<std::string, SimObjectInfo> simObjects_;           ///< The SimObjects in the repository.
    std::map<std::string, std::string> tagIndex_;               ///< Index of tags to SimObject IDs.
    std::map<std::string, std::set<std::string>> titleIndex_;   ///< Index of SimObjects IDs by title.

public:
    SimObjectRepository(std::filesystem::path repositoryPath)
        : repositoryPath_(std::move(repositoryPath))
    {
    }
    ~SimObjectRepository() = default;


    /**
     * Returns the path to the SimObject repository.
     * 
     * @returns The path to the SimObject repository.
     */
    const std::filesystem::path& repositoryPath() const noexcept {
        return repositoryPath_;
    }


    /**
     * Adds or updates a SimObject in the repository.
     * 
     * @param info The SimObject information. If the id is empty, a new UUID will be generated.
     * @return The id of the added/updated SimObject.
     */
    std::string setSimObject(SimObjectInfo info) {
        // Generate UUID if not provided
        if (info.id.empty()) {
            info.id = uuids::to_string(uuids::uuid_system_generator{}());
        }

        const auto id = info.id;

        // If updating an existing SimObject, clean up old indices
        auto existingIt = simObjects_.find(id);
        if (existingIt != simObjects_.end()) {
            const auto& oldInfo = existingIt->second;

            // Remove old tag from index
            if (oldInfo.tag.has_value()) {
                tagIndex_.erase(*oldInfo.tag);
            }

            // Remove old title from index
            auto titleIt = titleIndex_.find(oldInfo.title);
            if (titleIt != titleIndex_.end()) {
                titleIt->second.erase(id);
                if (titleIt->second.empty()) {
                    titleIndex_.erase(titleIt);
                }
            }
        }

        // Add to tag index if tag is present
        if (info.tag.has_value()) {
            tagIndex_[*info.tag] = id;
        }

        // Add to title index
        titleIndex_[info.title].insert(id);

        // Store or update the SimObject
        simObjects_.insert_or_assign(id, std::move(info));

        return id;
    }


    /**
     * Gets a SimObject by its unique identifier.
     * 
     * @param id The unique identifier of the SimObject.
     * @return The SimObject information, or std::nullopt if not found.
     */
    std::optional<SimObjectInfo> getById(const std::string& id) const {
        auto it = simObjects_.find(id);
        if (it != simObjects_.end()) {
            return it->second;
        }
        return std::nullopt;
    }


    /**
     * Gets a SimObject by its tag.
     * 
     * @param tag The tag of the SimObject.
     * @return The SimObject information, or std::nullopt if not found.
     */
    std::optional<SimObjectInfo> getByTag(const std::string& tag) const {
        auto tagIt = tagIndex_.find(tag);
        if (tagIt != tagIndex_.end()) {
            return getById(tagIt->second);
        }
        return std::nullopt;
    }


    /**
     * Gets all SimObjects with a given title.
     * 
     * @param title The title to search for.
     * @return A set of SimObject IDs with the given title.
     */
    std::set<std::string> getIdsByTitle(const std::string& title) const {
        auto it = titleIndex_.find(title);
        if (it != titleIndex_.end()) {
            return it->second;
        }
        return {};
    }


    /**
     * Checks if a SimObject with the given ID exists.
     * 
     * @param id The unique identifier to check.
     * @return true if the SimObject exists, false otherwise.
     */
    bool hasId(const std::string& id) const noexcept {
        return simObjects_.find(id) != simObjects_.end();
    }


    /**
     * Checks if a SimObject with the given tag exists.
     * 
     * @param tag The tag to check.
     * @return true if the tag exists, false otherwise.
     */
    bool hasTag(const std::string& tag) const noexcept {
        return tagIndex_.find(tag) != tagIndex_.end();
    }


    /**
     * Returns the number of SimObjects in the repository.
     * 
     * @return The count of SimObjects.
     */
    size_t size() const noexcept {
        return simObjects_.size();
    }


    /**
     * Checks if the repository is empty.
     * 
     * @return true if the repository contains no SimObjects, false otherwise.
     */
    bool empty() const noexcept {
        return simObjects_.empty();
    }


    /**
     * Returns all SimObjects in the repository.
     * 
     * @return A const reference to the map of all SimObjects.
     */
    const std::map<std::string, SimObjectInfo>& all() const noexcept {
        return simObjects_;
    }


    /**
     * Loads SimObjects from a simplified YAML file.
     * 
     * @param filePath The path to the YAML file to load. If not provided, uses repositoryPath_/simobjects.yaml
     * @return true if loaded successfully, false otherwise.
     */
    bool load(std::optional<std::filesystem::path> filePath = std::nullopt) {
        auto path = filePath.value_or(repositoryPath_ / "simobjects.yaml");
        
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        simObjects_.clear();
        tagIndex_.clear();
        titleIndex_.clear();

        SimObjectInfo current;
        std::string line;
        bool inObject = false;

        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Check for object start (line starts with "- id:")
            if (line.starts_with("- id:")) {
                // Save previous object if exists
                if (inObject && !current.id.empty()) {
                    setSimObject(std::move(current));
                    current = SimObjectInfo{};
                }
                inObject = true;
                current.id = trim(line.substr(5));
            }
            else if (inObject) {
                if (line.starts_with("  tag:")) {
                    auto value = trim(line.substr(6));
                    current.tag = value.empty() ? std::nullopt : std::optional<std::string>{value};
                }
                else if (line.starts_with("  type:")) {
                    auto value = trim(line.substr(7));
                    current.type = stringToSimObjectType(value);
                }
                else if (line.starts_with("  title:")) {
                    current.title = trim(line.substr(8));
                }
                else if (line.starts_with("  livery:")) {
                    auto value = trim(line.substr(9));
                    current.livery = value.empty() ? std::nullopt : std::optional<std::string>{value};
                }
            }
        }

        // Save last object
        if (inObject && !current.id.empty()) {
            setSimObject(std::move(current));
        }

        return true;
    }


    /**
     * Saves SimObjects to a simplified YAML file.
     * 
     * @param filePath The path to the YAML file to save. If not provided, uses repositoryPath_/simobjects.yaml
     * @return true if saved successfully, false otherwise.
     */
    bool save(std::optional<std::filesystem::path> filePath = std::nullopt) const {
        auto path = filePath.value_or(repositoryPath_ / "simobjects.yaml");
        
        // Create directory if it doesn't exist
        if (auto parent = path.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << "# SimObject Repository\n";
        file << "# Generated by CppSimConnect\n\n";

        for (const auto& [id, info] : simObjects_) {
            file << "- id: " << info.id << "\n";
            
            if (info.tag.has_value()) {
                file << "  tag: " << *info.tag << "\n";
            }
            
            file << "  type: " << simObjectTypeToString(info.type) << "\n";
            file << "  title: " << info.title << "\n";
            
            if (info.livery.has_value()) {
                file << "  livery: " << *info.livery << "\n";
            }
            
            file << "\n";
        }

        return true;
    }

private:
    static std::string trim(const std::string& str) {
        auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    static SimObjectType stringToSimObjectType(const std::string& str) {
        if (str == "aircraft") return SimObjectTypes::aircraft;
        if (str == "helicopter") return SimObjectTypes::helicopter;
        if (str == "boat") return SimObjectTypes::boat;
        if (str == "ground") return SimObjectTypes::ground;
#if MSFS_2024_SDK
        if (str == "hotAirBalloon") return SimObjectTypes::hotAirBalloon;
        if (str == "animal") return SimObjectTypes::animal;
        if (str == "userAvatar") return SimObjectTypes::userAvatar;
#endif
        return SimObjectTypes::aircraft; // default
    }

    static std::string simObjectTypeToString(SimObjectType type) {
        if (type == SimObjectTypes::aircraft) return "aircraft";
        if (type == SimObjectTypes::helicopter) return "helicopter";
        if (type == SimObjectTypes::boat) return "boat";
        if (type == SimObjectTypes::ground) return "ground";
#if MSFS_2024_SDK
        if (type == SimObjectTypes::hotAirBalloon) return "hotAirBalloon";
        if (type == SimObjectTypes::animal) return "animal";
        if (type == SimObjectTypes::userAvatar) return "userAvatar";
#endif
        return "aircraft"; // default
    }
};

} // namespace SimConnect::AI