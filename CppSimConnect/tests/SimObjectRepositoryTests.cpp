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

#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include <simconnect/simconnect.hpp>
#include <simconnect/ai/simobjects/simobject_repository.hpp>


using namespace SimConnect;
using namespace SimConnect::AI;

/**
 * Test fixture for SimObjectRepository tests.
 * Provides a temporary directory for file operations that is cleaned up after each test.
 */
class SimObjectRepositoryTest : public ::testing::Test {
protected:
    std::filesystem::path testPath;

    void SetUp() override {
        testPath = std::filesystem::temp_directory_path() / "simobject_repo_test";
        std::filesystem::create_directories(testPath);
    }

    void TearDown() override {
        if (std::filesystem::exists(testPath)) {
            std::filesystem::remove_all(testPath);
        }
    }
};

/**
 * Tests that a newly constructed repository has the correct path and is empty.
 */
TEST_F(SimObjectRepositoryTest, DefaultConstructorAndPath) {
    const SimObjectRepository repo(testPath);
    ASSERT_EQ(repo.repositoryPath(), testPath);
    ASSERT_EQ(repo.size(), 0U);
    ASSERT_TRUE(repo.empty());
}

/**
 * Tests that adding a SimObject without providing an ID generates a valid UUID.
 */
TEST_F(SimObjectRepositoryTest, AddSimObjectWithoutId) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.title = "Test Aircraft";
    info.type = SimObjectTypes::aircraft;
    
    const auto objectId = repo.setSimObject(info);
    
    ASSERT_FALSE(objectId.empty());
    ASSERT_EQ(repo.size(), 1U);
    ASSERT_FALSE(repo.empty());
    ASSERT_TRUE(repo.hasId(objectId));
}

/**
 * Tests that adding a SimObject with a specific ID preserves that ID.
 */
TEST_F(SimObjectRepositoryTest, AddSimObjectWithId) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "test-id-12345";
    info.title = "Test Aircraft";
    info.type = SimObjectTypes::aircraft;
    
    const auto objectId = repo.setSimObject(info);
    
    ASSERT_EQ(objectId, "test-id-12345");
    ASSERT_EQ(repo.size(), 1U);
    ASSERT_TRUE(repo.hasId("test-id-12345"));
}

/**
 * Tests that a SimObject can be successfully retrieved by its ID.
 */
TEST_F(SimObjectRepositoryTest, GetByIdExists) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "aircraft-001";
    info.title = "Cessna 172";
    info.type = SimObjectTypes::aircraft;
    
    repo.setSimObject(info);
    
    const auto retrieved = repo.getById("aircraft-001");

    if (retrieved.has_value()) {
        ASSERT_EQ(retrieved.value().id, "aircraft-001");
        ASSERT_EQ(retrieved.value().title, "Cessna 172");
        ASSERT_EQ(retrieved.value().type, SimObjectTypes::aircraft);
    }
    else {
        FAIL() << "SimObject not found by ID";
    }
}

/**
 * Tests that querying for a non-existent ID returns std::nullopt.
 */
TEST_F(SimObjectRepositoryTest, GetByIdNotExists) {
    const SimObjectRepository repo(testPath);
    
    const auto retrieved = repo.getById("nonexistent");
    ASSERT_FALSE(retrieved.has_value());
}

/**
 * Tests that a SimObject with a tag can be added and retrieved using that tag.
 */
TEST_F(SimObjectRepositoryTest, AddSimObjectWithTag) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "heli-001";
    info.tag = "my-helicopter";
    info.title = "Bell 407";
    info.type = SimObjectTypes::helicopter;
    
    repo.setSimObject(info);
    
    ASSERT_TRUE(repo.hasTag("my-helicopter"));
    
    const auto retrieved = repo.getByTag("my-helicopter");
    if (retrieved.has_value()) {
        ASSERT_EQ(retrieved.value().id, "heli-001");
        ASSERT_EQ(retrieved.value().title, "Bell 407");
    }
    else {
        FAIL() << "SimObject not found by tag";
    }
}
/**
 * Tests that querying for a non-existent tag returns std::nullopt.
 */
TEST_F(SimObjectRepositoryTest, GetByTagNotExists) {
    const SimObjectRepository repo(testPath);
    
    const auto retrieved = repo.getByTag("nonexistent-tag");
    ASSERT_FALSE(retrieved.has_value());
    ASSERT_FALSE(repo.hasTag("nonexistent-tag"));
}

/**
 * Tests that updating a SimObject's tag properly removes the old tag from the index
 * and adds the new tag.
 */
TEST_F(SimObjectRepositoryTest, UpdateSimObjectChangesTag) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "aircraft-001";
    info.tag = "old-tag";
    info.title = "Test Aircraft";
    info.type = SimObjectTypes::aircraft;
    
    repo.setSimObject(info);
    ASSERT_TRUE(repo.hasTag("old-tag"));
    
    // Update with new tag
    info.tag = "new-tag";
    repo.setSimObject(info);
    
    ASSERT_FALSE(repo.hasTag("old-tag"));
    ASSERT_TRUE(repo.hasTag("new-tag"));
    ASSERT_EQ(repo.size(), 1U); // Still only one object
}

/**
 * Tests that multiple SimObjects with the same title can be retrieved,
 * and that different titles return different sets of IDs.
 */
TEST_F(SimObjectRepositoryTest, GetIdsByTitle) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info1;
    info1.id = "aircraft-001";
    info1.title = "Cessna 172";
    info1.type = SimObjectTypes::aircraft;
    repo.setSimObject(info1);
    
    SimObjectInfo info2;
    info2.id = "aircraft-002";
    info2.title = "Cessna 172";
    info2.type = SimObjectTypes::aircraft;
    repo.setSimObject(info2);
    
    SimObjectInfo info3;
    info3.id = "aircraft-003";
    info3.title = "Piper Cub";
    info3.type = SimObjectTypes::aircraft;
    repo.setSimObject(info3);
    
    const auto cessnaIds = repo.getIdsByTitle("Cessna 172");
    ASSERT_EQ(cessnaIds.size(), 2U);
    ASSERT_TRUE(cessnaIds.contains("aircraft-001"));
    ASSERT_TRUE(cessnaIds.contains("aircraft-002"));
    
    const auto piperIds = repo.getIdsByTitle("Piper Cub");
    ASSERT_EQ(piperIds.size(), 1U);
    ASSERT_TRUE(piperIds.contains("aircraft-003"));
}

/**
 * Tests that querying for a non-existent title returns an empty set.
 */
TEST_F(SimObjectRepositoryTest, GetIdsByTitleNotExists) {
    const SimObjectRepository repo(testPath);
    
    const auto ids = repo.getIdsByTitle("Nonexistent Aircraft");
    ASSERT_TRUE(ids.empty());
}

/**
 * Tests that updating a SimObject's title properly removes it from the old title index
 * and adds it to the new title index.
 */
TEST_F(SimObjectRepositoryTest, UpdateSimObjectChangesTitle) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "aircraft-001";
    info.title = "Old Title";
    info.type = SimObjectTypes::aircraft;
    repo.setSimObject(info);
    
    // Update with new title
    info.title = "New Title";
    repo.setSimObject(info);
    
    ASSERT_TRUE(repo.getIdsByTitle("Old Title").empty());
    ASSERT_EQ(repo.getIdsByTitle("New Title").size(), 1U);
    ASSERT_EQ(repo.size(), 1U); // Still only one object
}

/**
 * Tests that the all() method returns a map containing all SimObjects in the repository.
 */
TEST_F(SimObjectRepositoryTest, AllReturnsAllObjects) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info1;
    info1.id = "aircraft-001";
    info1.title = "Aircraft 1";
    repo.setSimObject(info1);
    
    SimObjectInfo info2;
    info2.id = "heli-001";
    info2.title = "Helicopter 1";
    info2.type = SimObjectTypes::helicopter;
    repo.setSimObject(info2);
    
    const auto& allObjects = repo.all();
    ASSERT_EQ(allObjects.size(), 2U);
    ASSERT_TRUE(allObjects.contains("aircraft-001"));
    ASSERT_TRUE(allObjects.contains("heli-001"));
}

/**
 * Tests that an empty repository can be saved and loaded without errors.
 */
TEST_F(SimObjectRepositoryTest, SaveAndLoadEmptyRepository) {
    const SimObjectRepository repo(testPath);
    
    const auto savePath = testPath / "test_save.yaml";
    ASSERT_TRUE(repo.save(savePath));
    ASSERT_TRUE(std::filesystem::exists(savePath));
    
    SimObjectRepository repo2(testPath);
    ASSERT_TRUE(repo2.load(savePath));
    ASSERT_EQ(repo2.size(), 0U);
}

/**
 * Tests that a single SimObject with all fields populated can be saved to
 * and loaded from a YAML file, preserving all data.
 */
TEST_F(SimObjectRepositoryTest, SaveAndLoadSingleObject) { // NOLINT(readability-function-cognitive-complexity)
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "aircraft-001";
    info.tag = "test-tag";
    info.title = "Test Aircraft";
    info.type = SimObjectTypes::aircraft;
    info.livery = "Blue Livery";
    repo.setSimObject(info);
    
    const auto savePath = testPath / "test_save.yaml";
    ASSERT_TRUE(repo.save(savePath));
    
    SimObjectRepository repo2(testPath);
    ASSERT_TRUE(repo2.load(savePath));
    ASSERT_EQ(repo2.size(), 1U);
    
    const auto loaded = repo2.getById("aircraft-001");

    if (loaded.has_value()) {
        ASSERT_EQ(loaded.value().id, "aircraft-001");
        ASSERT_EQ(loaded.value().tag, "test-tag");
        ASSERT_EQ(loaded.value().title, "Test Aircraft");
        ASSERT_EQ(loaded.value().type, SimObjectTypes::aircraft);
        ASSERT_EQ(loaded.value().livery, "Blue Livery");
    }
    else {
        FAIL() << "SimObject not found by ID after loading";
    }
}

/**
 * Tests that multiple SimObjects of different types can be saved and loaded,
 * preserving all data and indices.
 */
TEST_F(SimObjectRepositoryTest, SaveAndLoadMultipleObjects) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info1;
    info1.id = "aircraft-001";
    info1.title = "Cessna 172";
    info1.type = SimObjectTypes::aircraft;
    repo.setSimObject(info1);
    
    SimObjectInfo info2;
    info2.id = "heli-001";
    info2.tag = "my-heli";
    info2.title = "Bell 407";
    info2.type = SimObjectTypes::helicopter;
    repo.setSimObject(info2);
    
    SimObjectInfo info3;
    info3.id = "boat-001";
    info3.title = "Speedboat";
    info3.type = SimObjectTypes::boat;
    info3.livery = "Racing Stripes";
    repo.setSimObject(info3);
    
    const auto savePath = testPath / "test_save.yaml";
    ASSERT_TRUE(repo.save(savePath));
    
    SimObjectRepository repo2(testPath);
    ASSERT_TRUE(repo2.load(savePath));
    ASSERT_EQ(repo2.size(), 3U);
    
    ASSERT_TRUE(repo2.hasId("aircraft-001"));
    ASSERT_TRUE(repo2.hasId("heli-001"));
    ASSERT_TRUE(repo2.hasId("boat-001"));
    ASSERT_TRUE(repo2.hasTag("my-heli"));
}

/**
 * Tests that attempting to load a non-existent file returns false
 * and leaves the repository empty.
 */
TEST_F(SimObjectRepositoryTest, LoadNonexistentFile) {
    SimObjectRepository repo(testPath);
    
    const auto fakePath = testPath / "nonexistent.yaml";
    ASSERT_FALSE(repo.load(fakePath));
    ASSERT_EQ(repo.size(), 0U);
}

/**
 * Tests that save() without a path argument uses the default path
 * (repositoryPath/simobjects.yaml).
 */
TEST_F(SimObjectRepositoryTest, SaveWithDefaultPath) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "test-001";
    info.title = "Test";
    repo.setSimObject(info);
    
    ASSERT_TRUE(repo.save()); // Uses default path
    
    const auto defaultPath = testPath / "simobjects.yaml";
    ASSERT_TRUE(std::filesystem::exists(defaultPath));
}

/**
 * Tests that load() without a path argument uses the default path
 * (repositoryPath/simobjects.yaml).
 */
TEST_F(SimObjectRepositoryTest, LoadWithDefaultPath) {
    const auto defaultPath = testPath / "simobjects.yaml";
    
    // Create a test file
    std::ofstream file(defaultPath);
    file << "- id: test-001\n";
    file << "  type: aircraft\n";
    file << "  title: Test Aircraft\n";
    file.close();
    
    SimObjectRepository repo(testPath);
    ASSERT_TRUE(repo.load()); // Uses default path
    ASSERT_EQ(repo.size(), 1U);
    ASSERT_TRUE(repo.hasId("test-001"));
}

/**
 * Tests that the YAML loader correctly skips comments (lines starting with #)
 * and empty lines.
 */
TEST_F(SimObjectRepositoryTest, LoadHandlesCommentsAndEmptyLines) {
    const auto testFile = testPath / "test.yaml";
    
    std::ofstream file(testFile);
    file << "# This is a comment\n";
    file << "\n";
    file << "- id: aircraft-001\n";
    file << "  type: aircraft\n";
    file << "  title: Test Aircraft\n";
    file << "\n";
    file << "# Another comment\n";
    file.close();
    
    SimObjectRepository repo(testPath);
    ASSERT_TRUE(repo.load(testFile));
    ASSERT_EQ(repo.size(), 1U);
}

/**
 * Tests that the title index correctly handles multiple SimObjects
 * sharing the same title but having different IDs.
 */
TEST_F(SimObjectRepositoryTest, MultipleObjectsWithSameTitleDifferentIds) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info1;
    info1.id = "id-001";
    info1.title = "Same Title";
    repo.setSimObject(info1);
    
    SimObjectInfo info2;
    info2.id = "id-002";
    info2.title = "Same Title";
    repo.setSimObject(info2);
    
    const auto ids = repo.getIdsByTitle("Same Title");
    ASSERT_EQ(ids.size(), 2U);
    ASSERT_TRUE(ids.contains("id-001"));
    ASSERT_TRUE(ids.contains("id-002"));
}

/**
 * Tests that a SimObject with only required fields (no tag, no livery)
 * can be stored and retrieved correctly.
 */
TEST_F(SimObjectRepositoryTest, SimObjectWithOptionalFields) {
    SimObjectRepository repo(testPath);
    
    SimObjectInfo info;
    info.id = "minimal-001";
    info.title = "Minimal Aircraft";
    // No tag, no livery
    
    repo.setSimObject(info);
    
    const auto retrieved = repo.getById("minimal-001");

    if (retrieved.has_value()) {
        ASSERT_FALSE(retrieved.value().tag.has_value());
        ASSERT_FALSE(retrieved.value().livery.has_value());
        ASSERT_EQ(retrieved.value().title, "Minimal Aircraft");
    }
    else {
        FAIL() << "SimObject not found by ID";
    }
}
