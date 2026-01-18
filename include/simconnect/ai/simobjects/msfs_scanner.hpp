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

#include <cstdlib>

#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <regex>

#include <simconnect/simconnect.hpp>
#include <simconnect/ai/simobjects/ini_file.hpp>

#include <simconnect/util/logger.hpp>
#include <simconnect/util/null_logger.hpp>


namespace SimConnect::AI {


template <class L = SimConnect::NullLogger>
class MSFSScanner {
public:
    using logger_type = L;

private:
    logger_type logger_;
    std::atomic<bool> scanDone_{ false };
    std::map<std::string, std::string> baseCategories_;
    std::map<std::string, std::string> unresolvedChildren_;
    std::map<SimObjectType, std::set<std::string>> titles_;


    /**
     * Adds a title to the scanner.
     * 
     * @param category The category of the aircraft.
     * @param title The title of the aircraft.
     */
    void addTitle(std::string category, std::string title) {
        if (category.empty()) {
            logger_.error("Cannot add a SimObject without category: '{}'", title);
            return;
        }
        category = toLower(category);

        SimObjectType type;
        if (category == "airplane") {
            type = SimObjectType::SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT;
        } else if (category == "helicopter") {
            type = SimObjectType::SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER;
        } else if (category == "boat") {
            type = SimObjectType::SIMCONNECT_SIMOBJECT_TYPE_BOAT;
        } else if (category == "groundvehicle") {
            type = SimObjectType::SIMCONNECT_SIMOBJECT_TYPE_GROUND;
        } else if (category == "viewer") {
            logger_.trace("Skipping viewer  '{}'", title);
        } else if (category == "staticobject") {
            logger_.trace("Skipping static object '{}'", title);
        } else if ((category == "flyinganimal") || (category == "animal")) {
            logger_.trace("Skipping animal '{}' (MSFS 2024 feature)", title);
        } else if ((category == "aircraftpilot") || (category == "human")) {
            logger_.trace("Skipping human '{}' (MSFS 2024 feature)", title);
        } else {
            logger_.error("Unknown category '{}' for SimObject '{}'", category, title);
            return;
        }
        titles_[type].insert(title);
        logger_.debug("Found SimObject: '{}' (category='{}')", title, category);
    }

    // package/SimObjects/Airplanes/xyz/aircraft.cfg
    // package/SimObjects/Misc/xyz/sim.cfg


    /**
     * Processes a SimObject's configuration file.
     * 
     * @param cfgPath The path to the configuration file.
     */
    void processSimObjectCfg(const std::filesystem::path& cfgPath, std::string baseName)
    {
        logger_.trace("Processing {}", cfgPath.string());
        IniFile ini;
        ini.load(cfgPath);

        // Read GENERAL.category once per file as default category
        std::string category{};
        std::string baseContainer{};

        if (auto cat = ini.get("general", "category")) {
            category = *cat;
            baseCategories_.emplace(baseName, category);
        } else if (auto base = ini.get("variation", "base_container")) {
            baseContainer = std::filesystem::path(*base).filename().string();
        } else {
            logger_.warn("No \"category\" found in section \"general\" of {}", cfgPath.string());
            return;
        }

        // Iterate all sections and pick those matching fltsim.N
        std::regex fltsimRe(R"(^fltsim\.\d+$)", std::regex::icase);
        for (const auto& [secName, sec] : ini.sections()) {
            if (!std::regex_match(secName, fltsimRe)) {
                continue;
            }

            // Required: title
            if (!sec.contains("title")) {
                logger_.warn("Missing title in {}", cfgPath.string());
                continue;
            }

            const auto& title = sec.at("title");
            if (!category.empty()) {
                // Use the category from GENERAL
                addTitle(category, title);
                logger_.trace("Added {} '{}'", category, title);
            } else if (!baseContainer.empty()) {
                // Try to resolve category via base container
                if (baseCategories_.contains(baseContainer)) {
                    const auto& resolvedCategory = baseCategories_.at(baseContainer);
                    addTitle(resolvedCategory, title);
                    logger_.trace("Added {} '{}'", resolvedCategory, title);
                } else {
                    // Store for later resolution
                    unresolvedChildren_.emplace(title, baseContainer);
                    logger_.trace("Stored unresolved '{}' with base container '{}'", title, baseContainer);
                }
            } else {
                logger_.warn("No category or base_container found for title '{}' in {}", title, cfgPath.string());
            }
        }
    }


    /**
     * Scans a package for SimObjects.
     * 
     * @param categoryRoot The root path of the SimObjects in the package.
     */
    void scanCategory(const std::filesystem::path& categoryRoot)
    {
        logger_.debug("Scanning category root: {}", categoryRoot.string());

        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(categoryRoot, ec)) {
            if (ec) {
                logger_.error("Error iterating directory {}: {}", categoryRoot.string(), ec.message());
                return;
            }

            if (entry.is_directory()) {
                const auto simObjectDir = entry.path();
                logger_.debug("Checking SimObject directory: {}", simObjectDir.string());
                if (const auto aircraftCfg = entry.path() / "aircraft.cfg"; std::filesystem::exists(aircraftCfg) && std::filesystem::is_regular_file(aircraftCfg)) {
                    processSimObjectCfg(aircraftCfg, entry.path().filename().string());
                } else if (const auto simCfg = entry.path() / "sim.cfg"; std::filesystem::exists(simCfg) && std::filesystem::is_regular_file(simCfg)) {
                    processSimObjectCfg(simCfg, entry.path().filename().string());
                } else {
                    logger_.trace("No SimObject configuration found in {}", entry.path().string());
                }
            }
        }
    }


    /**
     * Scans a root directory for packages containing SimObjects.
     * 
     * @param root The root directory to scan.
     */
    void scanTree(const std::filesystem::path& root) {
        logger_.debug("Scanning root: {}", root.string());
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (entry.is_directory()) {
                const auto simObjectPath = entry.path() / "SimObjects";
                if (std::filesystem::exists(simObjectPath) && std::filesystem::is_directory(simObjectPath)) {
                    // Scan the SimObject categories in this package
                    for (const auto& categoryEntry : std::filesystem::directory_iterator(simObjectPath)) {
                        if (categoryEntry.is_directory()) {
                            scanCategory(categoryEntry.path());
                        }
                    }
                }
            }
        }
    }

#pragma warning(push)
#pragma warning(disable : 4996)

    /**
     * Returns the user configuration file path, or an empty path if not found.
     * 
     * @returns The user configuration file path.
     */
    std::filesystem::path userCfgPath()
    {
        const auto appData = std::getenv("APPDATA");          // C:\Users\<User>\AppData\Roaming
        if (appData != nullptr) {
            const std::filesystem::path appDataPath = appData;

            if (const auto userCfg = appDataPath / "Microsoft Flight Simulator" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
                return userCfg;
            }
        }

        const char* package = "Microsoft.FlightSimulator_8wekyb3d8bbwe";
        const auto localAppData = std::getenv("LOCALAPPDATA"); // C:\Users\<User>\AppData\Local

        if (localAppData != nullptr) {
            const std::filesystem::path localAppDataPath = localAppData;

            // MS Store typical
            if (const auto userCfg = localAppDataPath / "Packages/" / package / "LocalCache" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
                return userCfg;
            }
            if (const auto userCfg = localAppDataPath / "Packages/" / package / "LocalState" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
                return userCfg;
            }
        }

        const auto userProfile = std::getenv("USERPROFILE");

        if (userProfile != nullptr) {
            const std::filesystem::path userProfilePath = userProfile;

            // Fallbacks via USERPROFILE
            if (const auto userCfg = userProfilePath / "AppData" / "Roaming" / "Microsoft Flight Simulator" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
                return userCfg;
            }
            if (const auto userCfg = userProfilePath / "AppData" / "Local" / "Packages" / package / "LocalCache" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
                return userCfg;
            }
        }
        // Steam common default
        const auto steamApps = std::filesystem::path("C:/Program Files (x86)/Steam/steamapps/common");
        if (const auto userCfg = steamApps / "MicrosoftFlightSimulator" / "UserCfg.opt"; std::filesystem::exists(userCfg)) {
            return userCfg;
        }

        return {};
    }
#pragma warning(pop)

    /**
     * Extracts the InstalledPackagesPath from the UserCfg.opt file.
     * 
     * @returns The InstalledPackagesPath, or an empty path if not found.
     */
    std::filesystem::path installedPackagesPath()
    {
        const auto userCfg = userCfgPath();
        if (userCfg.empty()) {
            logger_.error("UserCfg.opt not found");
            return {};
        }
        std::ifstream in(userCfg);
        if (!in) {
            logger_.error("Failed to open UserCfg.opt at {}", userCfg.string());
            return {};
        }
        std::string line;
        const std::string key = "InstalledPackagesPath";
        while (std::getline(in, line)) {
            if (line.starts_with(key)) {
                const auto path = trim(line.substr(key.size()), true);

                if (!path.empty()) {
                    std::filesystem::path p = path;
                    if (std::filesystem::exists(p)) {
                        logger_.info("Found InstalledPackagesPath: {}", p.string());
                        return p;
                    } else {
                        logger_.error("InstalledPackagesPath '{}' does not exist", p.string());
                        return {};
                    }
                }
            }
        }
        logger_.error("InstalledPackagesPath not found in UserCfg.opt");
        return {};
    }


public:
    MSFSScanner(std::string loggerName = "SimConnect::ai::MSFSScanner", LogLevel logLevel = LogLevel::Info)
        : logger_(loggerName, logLevel)
    {
    }

    ~MSFSScanner() = default;


    /**
     * Returns the logger.
     * 
     * @returns The logger.
     */
    logger_type& logger() noexcept { return logger_; }


    /**
     * Scans for SimObjects of the specified type and invokes the callback for each found title and livery.
     * 
     * @param type The type of SimObject to scan for.
     * @param callback The callback to invoke for each found title and livery.
     */
    void scan(SimObjectType type, std::function<void (std::string_view title, std::string_view livery)> callback) {
        if (!scanDone_) {
            const auto installed = installedPackagesPath();
            if (installed.empty()) {
                logger_.error("Cannot scan SimObjects: InstalledPackagesPath not found");
                return;
            }

            // Official packages
            const auto official = installed / "Official" / "OneStore";
            if (std::filesystem::exists(official) && std::filesystem::is_directory(official)) {
                scanTree(official);
            }

            // Community packages
            const auto community = installed / "Community";
            if (std::filesystem::exists(community) && std::filesystem::is_directory(community)) {
                scanTree(community);
            }

            // Resolve any unresolved children
            for (const auto& [title, baseContainer] : unresolvedChildren_) {
                if (baseCategories_.contains(baseContainer)) {
                    const auto& resolvedCategory = baseCategories_.at(baseContainer);
                    addTitle(resolvedCategory, title);
                    logger_.trace("Resolved and added {} '{}'", resolvedCategory, title);
                } else {
                    logger_.warn("No base container '{}' with a category for title '{}'", baseContainer, title);
                }
            }

            scanDone_ = true;
        }

        if (titles_.contains(type)) {
            for (const auto& title : titles_.at(type)) {
                callback(title, "");
            }
        }
        else {
            logger_.warn("No titles found for SimObject type {}", static_cast<int>(type));
        }
    }
};

} // namespace SimConnect::AI