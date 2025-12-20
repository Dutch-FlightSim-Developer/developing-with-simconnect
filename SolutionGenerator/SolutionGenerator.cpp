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


 // Disable warnings
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <iostream>
#include <string>
#include <array>
#include <set>

#include <cstdlib>
#include <cstring>


constexpr static const char* optOutput = "-o";
constexpr static const char* optOutPutLong = "--output";
constexpr static const char* optTemplate = "-t";
constexpr static const char* optTemplateLong = "--template-dir";
constexpr static const char* optHelp = "-h";
constexpr static const char* optHelpLong = "--help";
constexpr static const char* optArch = "-a";
constexpr static const char* optArchLong = "--arch";
constexpr static const char* optArchX86 = "x86";
constexpr static const char* optArchX64 = "x64";
constexpr static const char* optSim = "-s";
constexpr static const char* optSimLong = "--sim";
constexpr static const char* optSimFsx = "fsx";
constexpr static const char* optSimP3Dv4 = "p3d-v4";
constexpr static const char* optSimP3Dv5 = "p3d-v5";
constexpr static const char* optSimP3Dv6 = "p3d-v6";
constexpr static const char* optSimMSFS2020 = "msfs-2020";
constexpr static const char* optSimMSFS2024 = "msfs-2024";
constexpr static const char* optEnv = "-e";
constexpr static const char* optEnvLong = "--env";
constexpr static const char* optNoEnvCheck = "-n";
constexpr static const char* optNoEnvCheckLong = "--no-env-check";
constexpr static const char* envFsxSdk = "FSX_SDK";
constexpr static const char* envP3Dv4Sdk = "P3Dv4_SDK";
constexpr static const char* envP3Dv5Sdk = "P3Dv5_SDK";
constexpr static const char* envP3Dv6Sdk = "P3Dv6_SDK";
constexpr static const char* envMsfs2020Sdk = "MSFS_SDK";
constexpr static const char* envMsfs2024Sdk = "MSFS2024_SDK";


static std::string templateDir = "../templates";
static std::string outputDir = ".";
static std::string simName = optSimMSFS2024;
static std::string arch = optArchX64;
static std::string sdkEnv = envMsfs2024Sdk;


static const std::array<std::string, 3> templateFiles = {
	"SolutionGenerator.sln",
	"SolutionGenerator.vcxproj",
	"SolutionGenerator.vcxproj.filters"
};


/**
 * Print out usage information
 */
static void usage() {
	std::cerr << "SolutionGenerator [options] <solution name>\n"
		<< "  Options:\n"
		<< "    " << optOutput << " | " << optOutPutLong << " <output directory>\n"
		<< "    " << optTemplate << " | " << optTemplateLong << " <template directory>\n"
		<< "    " << optHelp << " | " << optHelpLong << "\n"
		<< "    " << optArch << " | " << optArchLong << " <x86 | x64>\n"
		<< "    " << optSim << " | " << optSimLong << " <fsx | p3d-v4 | p3d-v5 | p3d-v6 | msfs-2020 | msfs-2024>\n"
		<< "    " << optEnv << " | " << optEnvLong << " <sdk-environment-variable>\n"
		<< "    " << optNoEnvCheck << " | " << optNoEnvCheckLong << "\n"
		<< "\n"
		<< "  Defaults:\n"
		<< "  - Output directory        : " << outputDir << "\n"
		<< "  - Template directory      : " << templateDir << "\n"
		<< "  - Simulator               : " << simName << "\n"
		<< "  - Architecture            : " << arch << "\n"
		<< "  - SDK environment variable: " << sdkEnv << "\n";
}


/**
 * Parse command line options to find the (optional) template directory and the output directory.
 * 
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 * 
 * @return A string containing the name of the solution to generate
 */
std::string parseOpts(int argc, const char** argv) {
	int i = 1;
	while ((i < argc) && (argv[i][0] == '-')) {
		if (argc < (i + 1)) {
			std::cerr << "Missing argument for option: " << argv[i] << std::endl;
			throw std::runtime_error("Missing argument for option");
		}
		if ((std::strncmp(argv[i], optTemplate, std::strlen(optTemplate)) == 0) ||
			(std::strncmp(argv[i], optTemplateLong, std::strlen(optTemplateLong)) == 0)) {
			templateDir = argv[i+1];
		}
		else if ((std::strncmp(argv[i], optOutput, std::strlen(optOutput)) == 0) ||
				 (std::strncmp(argv[i], optOutPutLong, std::strlen(optOutPutLong)) == 0)) {
			outputDir = argv[i+1];
		}
		else if ((std::strncmp(argv[i], optHelp, std::strlen(optHelp)) == 0) ||
				 (std::strncmp(argv[i], optHelpLong, std::strlen(optHelpLong)) == 0)) {
			usage();
			exit(0);
		}
		else if ((std::strncmp(argv[i], optArch, std::strlen(optArch)) == 0) ||
				 (std::strncmp(argv[i], optArchLong, std::strlen(optArchLong)) == 0)) {
			arch = argv[i+1];
		}
		else if ((std::strncmp(argv[i], optSim, std::strlen(optSim)) == 0) ||
				 (std::strncmp(argv[i], optSimLong, std::strlen(optSimLong)) == 0)) {
			simName = argv[i+1];
		}
		else if ((std::strncmp(argv[i], optEnv, std::strlen(optEnv)) == 0) ||
				 (std::strncmp(argv[i], optEnvLong, std::strlen(optEnvLong)) == 0)) {
			sdkEnv = argv[i+1];
		}
		else if ((std::strncmp(argv[i], optNoEnvCheck, std::strlen(optNoEnvCheck)) == 0) ||
				 (std::strncmp(argv[i], optNoEnvCheckLong, std::strlen(optNoEnvCheckLong)) == 0)) {
			sdkEnv = "NO_CHECK";
		}
		else {
			std::cerr << "Unknown option: " << argv[1] << std::endl;
			throw std::runtime_error("Unknown option");
		}

		i += 2;
	}
	if (i >= argc) {
		std::cerr << "Missing solution name" << std::endl;
		throw std::runtime_error("Missing solution name");
	}
	return argv[i];
}


/**
 * Tell the user what we're going to do.
 */
static void printInfo(std::string solutionName) {
	std::cout
		<< "Generating solution: " << solutionName
		<< "\n  Template directory: " << templateDir
		<< "\n  Output directory: " << outputDir
		<< "\n  Simulator: " << simName
		<< "\n  Architecture: " << arch
		<< "\n  SDK environment variable: " << sdkEnv << "\n";
}


constexpr static const char* incDirFSX[] = { /*"Core Utilities Kit",*/ "SimConnect SDK", "inc", nullptr };
constexpr static const char* incDirP3D[] = { "inc", "SimConnect", nullptr };
constexpr static const char* incDirMSFS[] = { "SimConnect SDK", "include", nullptr };

constexpr static const char* libDirFSX[] = { /*"Core Utilities Kit",*/ "SimConnect SDK", "lib", nullptr };
constexpr static const char* libDirP3D[] = { "lib", "SimConnect",	nullptr };
constexpr static const char* libDirMSFS[] = { "SimConnect SDK", "lib", "static", nullptr };

constexpr static const char* incName = "SimConnect.h";
constexpr static const char* libNameDebug1 = "SimConnectDebug.lib";
constexpr static const char* libNameDebug2 = "SimConnect_debug.lib";
constexpr static const char* libNameRelease = "SimConnect.lib";

static std::string incDir;
static std::string libDir;
static std::string libName;


static std::filesystem::path buildPath(const std::filesystem::path& dir, const char* const elems[]) {
	std::filesystem::path path = dir;
	for (int i = 0; elems[i] != nullptr; i++) {
		path /= elems [i];
	}
	return path;
}


static std::string buildPath(const char* const elems[], char separator = '\\') {
	std::string result;
	for (int i = 0; elems[i] != nullptr; i++) {
		if (i > 0) {
			result += separator;
		}
		result += elems[i];
	}
	return result;
}


static bool checkFile(std::filesystem::path dir, const char* fileName) {
	std::cout << "Checking for " << fileName << " in " << dir << std::endl;

	if (!std::filesystem::exists(dir)) {
		std::cerr << "Directory " << dir << " does not exist" << std::endl;
		return false;
	}
	if (!std::filesystem::is_directory(dir)) {
		std::cerr << "Directory " << dir << " is not a directory" << std::endl;
		return false;
	}
	std::filesystem::path file = dir;
	file /= fileName;
	return std::filesystem::exists(file) && std::filesystem::is_regular_file(file);
}


static void checkSDK()
{
	const char* sdkDir = std::getenv(sdkEnv.c_str());
	if ((sdkDir == nullptr) || (sdkDir[0] == '\0')) {
		std::cerr << "Environment variable " << sdkEnv << " is not set" << std::endl;
		throw std::runtime_error("Environment variable " + sdkEnv + " is not set");
	}
	std::cout << "Checking for the SDK at " << sdkDir << std::endl;
	std::filesystem::path sdk = sdkDir;
	if (!std::filesystem::exists(sdk) || !std::filesystem::is_directory(sdk)) {
		std::cerr << "Environment variable " << sdkEnv << " does not point to a valid directory" << std::endl;
	}

	auto check = buildPath(sdk, incDirMSFS);

	if (checkFile(check, incName)) {
		std::cout << std::format("Found '{}' using MSFS SDK style.\n", incName);
		incDir = buildPath(incDirMSFS);
	}
	else {
		check = buildPath(sdk, incDirFSX);
		if (checkFile(check, incName)) {
			std::cout << std::format("Found '{}' using FSX SDK style.\n", incName);
			incDir = buildPath(incDirFSX);
		}
		else {
			check = buildPath(sdk, incDirP3D);
			if (checkFile(check, incName)) {
				std::cout << std::format("Found '{}' using P3D SDK style.\n", incName);
				incDir = buildPath(incDirP3D);
			}
			else {
				std::cerr << "Could not find " << incName << " in the SDK at '" << sdkDir << "'" << std::endl;
				throw std::runtime_error("Could not find SimConnect header in the SDK");
			}
		}
	}
}


const int testArgc = 4;
const char* testArgv[] = {
	"SolutionGenerator",
	"-e", "P3Dv5_SDK",
	"SolutionGenerator"
};

const std::set<std::string> simx32 = { optSimFsx, optSimP3Dv4 };

auto main(int argc, const char** argv) -> int {
	try {
		std::string solutionName = parseOpts(testArgc, testArgv);

		printInfo(solutionName);
		checkSDK();
	}
	catch (const std::runtime_error& e) {
		usage();
		return 1;
	}
	return 0;
}