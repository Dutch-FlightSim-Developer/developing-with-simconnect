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


#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/system_events.hpp>
#include <simconnect/util/console_logger.hpp>
#include <simconnect/util/logger.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data_definition.hpp>
#include <simconnect/requests/simobject_data_handler.hpp>


#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <string>


using namespace std::chrono_literals;


/**
 * Return a formatted string of the version. if the major number is 0, it returns "Unknown". The lower number is ignored
 * if 0.
 *
 * @param major The major version number.
 * @param minor The minor version number.
 * @returns a string with the formatted version number.
 */
inline static std::string version(unsigned long major, unsigned long minor)
{
  if (major == 0) { return "Unknown"; }
  return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


// Lots of references to constants defined in SimConnect.h
// NOLINTBEGIN(misc-include-cleaner)

/**
 * Handle an exception message, printing details to the standard error output.
 *
 * @param msg The exception message received.
 */
static void handleException(const SIMCONNECT_RECV_EXCEPTION &msg)
{
  const SIMCONNECT_EXCEPTION exc{ static_cast<SIMCONNECT_EXCEPTION>(msg.dwException) };
  std::cerr << std::format("Received an exception type {}:\n", (int)exc);
  if (msg.dwSendID != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_SENDID) {
    std::cerr << std::format("- Related to a message with SendID {}.\n", (int)msg.dwSendID);
  }
  if (msg.dwIndex != SIMCONNECT_RECV_EXCEPTION::UNKNOWN_INDEX) {
    std::cerr << std::format("- Regarding parameter {}.\n", (int)msg.dwIndex);
  }
  switch (exc) {
  case SIMCONNECT_EXCEPTION_NONE:// Should never happen
    std::cerr << "No exception.\n";
    break;
  case SIMCONNECT_EXCEPTION_ERROR:
    std::cerr << "Some unspecific error has occurred.\n";
    break;
  case SIMCONNECT_EXCEPTION_SIZE_MISMATCH:
    std::cerr << "The size of the parameter does not match the expected size.\n";
    break;
  case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:
    std::cerr << "The parameter is not a recognized ID.\n";
    break;
  case SIMCONNECT_EXCEPTION_UNOPENED:
    std::cerr << "The connection has not been opened.\n";
    break;
  case SIMCONNECT_EXCEPTION_VERSION_MISMATCH:
    std::cerr << "This version of SimConnect cannot work with this version of the simulator.\n";
    break;
  case SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS:
    std::cerr << "The maximum number of (input/notification) groups has been reached. (currently 20)\n";
    break;
  case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:
    std::cerr << "The parameter is not a recognized name.\n";
    break;
  case SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES:
    std::cerr << "The maximum number of event names has been reached. (currently 1000)\n";
    break;
  case SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE:
    std::cerr << "The event ID is already in use.\n";
    break;
  case SIMCONNECT_EXCEPTION_TOO_MANY_MAPS:
    std::cerr << "The maximum number of mapings has been reached. (currently 20)\n";
    break;
  case SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS:
    std::cerr << "The maximum number of objects has been reached. (currently 1000)\n";
    break;
  case SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS:
    std::cerr << "The maximum number of requests has been reached. (currently 1000)\n";
    break;
  case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:// Legacy
    std::cerr << "The weather port is invalid.\n";
    break;
  case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:// Legacy
    std::cerr << "The METAR string is invalid.\n";
    break;
  case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:// Legacy
    std::cerr << "Unable to get the observation.\n";
    break;
  case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:// Legacy
    std::cerr << "Unable to create the station.\n";
    break;
  case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:// Legacy
    std::cerr << "Unable to remove the station.\n";
    break;
  case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:
    std::cerr << "The requested data cannot be converted to the specified data type.\n";
    break;
  case SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE:
    std::cerr << "The requested data cannot be transferred in the specified data size.\n";
    break;
  case SIMCONNECT_EXCEPTION_DATA_ERROR:
    std::cerr << "The data passed is invalid.\n";
    break;
  case SIMCONNECT_EXCEPTION_INVALID_ARRAY:
    std::cerr << "The array passed to SetDataOnSimObject is invalid.\n";
    break;
  case SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED:
    std::cerr << "The AI object could not be created.\n";
    break;
  case SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED:
    std::cerr << "The flight plan could not be loaded. Either it could not be found, or it contained an error.\n";
    break;
  case SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE:
    std::cerr << "The operation is not valid for the object type.\n";
    break;
  case SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION:
    std::cerr << "The operation is illegal. (AI or Weather)\n";
    break;
  case SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED:
    std::cerr << "The client is already subscribed to this event.\n";
    break;
  case SIMCONNECT_EXCEPTION_INVALID_ENUM:
    std::cerr << "The type enum value is unknown. (Probably an unknown type in RequestDataOnSimObjectType)\n";
    break;
  case SIMCONNECT_EXCEPTION_DEFINITION_ERROR:
    std::cerr << "The definition is invalid. (Probably a variable length requested in RequestDataOnSimObject)\n";
    break;
  case SIMCONNECT_EXCEPTION_DUPLICATE_ID:
    std::cerr << "The ID is already in use. (Menu, DataDefinition item ID, ClientData mapping, or event to "
                 "notification group)\n";
    break;
  case SIMCONNECT_EXCEPTION_DATUM_ID:
    std::cerr << "Unknown datum ID specified for SetDataOnSimObject.\n";
    break;
  case SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS:
    std::cerr
      << "The requested value is out of bounds. (radius of a RequestDataOnSimObjectType, or CreateClientData)\n";
    break;
  case SIMCONNECT_EXCEPTION_ALREADY_CREATED:
    std::cerr << "A ClientData area with that name has already been created.\n";
    break;
  case SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE:
    std::cerr << "The AI object is outside the reality bubble.\n";
    break;
  case SIMCONNECT_EXCEPTION_OBJECT_CONTAINER:
    std::cerr << "The AI object creation failed. (container issue)\n";
    break;
  case SIMCONNECT_EXCEPTION_OBJECT_AI:
    std::cerr << "The AI object creation failed. (AI issue)\n";
    break;
  case SIMCONNECT_EXCEPTION_OBJECT_ATC:
    std::cerr << "The AI object creation failed. (ATC issue)\n";
    break;
  case SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE:
    std::cerr << "The AI object creation failed. (scheduling issue)\n";
    break;
  case SIMCONNECT_EXCEPTION_JETWAY_DATA:
    std::cerr << "Requesting JetWay data failed.\n";
    break;
  case SIMCONNECT_EXCEPTION_ACTION_NOT_FOUND:
    std::cerr << "The action was not found.\n";
    break;
  case SIMCONNECT_EXCEPTION_NOT_AN_ACTION:
    std::cerr << "The action was not a valid action.\n";
    break;
  case SIMCONNECT_EXCEPTION_INCORRECT_ACTION_PARAMS:
    std::cerr << "The action parameters were incorrect.\n";
    break;
  case SIMCONNECT_EXCEPTION_GET_INPUT_EVENT_FAILED:
    std::cerr << "The input event name was not found. (GetInputEvent)\n";
    break;
  case SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED:
    std::cerr << "The input event name was not found. (SetInputEvent)\n";
    break;
  case SIMCONNECT_EXCEPTION_INTERNAL:
    std::cerr << "An internal error has occurred.\n";
    break;
    // No default; we want an error if we miss one
  }
}

// NOLINTEND(misc-include-cleaner)

/**
 * Print the information of the "Open" message, which tells us some details about the simulator.
 *
 * @param msg The message received.
 */
static void handleOpen(const SIMCONNECT_RECV_OPEN &msg)
{// NOLINT(misc-include-cleaner)
  std::cout << "Connected to " << &msg.szApplicationName[0] << " version "
            << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << '\n'
            << "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << '\n'
            << "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor)
            << '\n'
            << "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << '\n';
}


/**
 * Tell the user the simulator is shutting down.
 *
 * @param msg The message received.
 */
static void handleClose([[maybe_unused]] const SIMCONNECT_RECV_QUIT &msg)
{// NOLINT(misc-include-cleaner)
  std::cout << "Simulator shutting down.\n";
}


constexpr static const char *appName = "List parkings";
static HANDLE hSimConnect{ nullptr };// The connection handle
static HANDLE hEvent{ nullptr };// The event handle (for efficient waiting for SimConnect messages)

static std::map<std::string, std::string> args;

// Map to store SendIDs for tracking
static std::map<DWORD, std::string> sendIdTracker;
static DWORD nextSendId = 1000;// Start from 1000 to avoid conflicts with existing IDs

constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_AIRCRAFT_INFO{ 1 };
constexpr static const SIMCONNECT_DATA_REQUEST_ID REQID_AIRCRAFT_POSITION{ 2 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_AIRCRAFT_INFO{ 1 };
constexpr static const SIMCONNECT_DATA_DEFINITION_ID DEFID_AIRCRAFT_POSITION{ 2 };

constexpr static SIMCONNECT_INPUT_GROUP_ID INPGRP_RECORD{ 1 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVT_TOGGLE_RECORDING{ 1 };
constexpr static SIMCONNECT_INPUT_GROUP_ID INPGRP_EXIT{ 2 };
constexpr static SIMCONNECT_CLIENT_EVENT_ID EVT_EXIT{ 2 };



#pragma pack(push, 1)// Ensure no padding bytes are added to the structure
struct AircraftInfo
{
  char title[128];// TITLE (String, max 128 chars)
  char livery[256];// LIVERY NAME (String, max 128 chars)

  double planeLatitude;// PLANE ALTITUDE (Degrees)
  double planeLongitude;// PLANE LONGITUDE (Degrees)
  double planeAltitude;// PLANE ALTITUDE (Feet)

  float planePitch;// PLANE PITCH DEGREES (Degrees)
  float planeBank;// PLANE BANK DEGREES (Degrees)
  float planeHeading;// PLANE HEADING DEGREES TRUE (Degrees)

  int32_t onGround;// SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)

  float planeAirspeed;// AIRSPEED TRUE (Knots)
};

struct AircraftPosition
{
  double planeLatitude;         // PLANE ALTITUDE (Degrees)
  double planeLongitude;        // PLANE LONGITUDE (Degrees)
  double planeAltitude;         // PLANE ALTITUDE (Feet)

  float planePitch;             // PLANE PITCH DEGREES (Degrees)
  float planeBank;              // PLANE BANK DEGREES (Degrees)
  float planeHeading;           // PLANE HEADING DEGREES TRUE (Degrees)

  float planeAirspeed;          // AIRSPEED INDICATED (Knots)

  float planeVelocityX;         // VELOCITY BODY X (Feet per second)
  float planeVelocityY;         // VELOCITY BODY Y (Feet per second)
  float planeVelocityZ;         // VELOCITY BODY Z (Feet per second)

  float planeAccelerationX;     // ACCELERATION BODY X (Feet per second squared)
  float planeAccelerationY;     // ACCELERATION BODY Y (Feet per second squared)
  float planeAccelerationZ;     // ACCELERATION BODY Z (Feet per second squared)

  float planeRotationVelocityX; // ROTATION VELOCITY BODY X (Degrees per second)
  float planeRotationVelocityY; // ROTATION VELOCITY BODY Y (Degrees per second)
  float planeRotationVelocityZ; // ROTATION VELOCITY BODY Z (Degrees per second)

  int32_t onGround;// SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)
};
#pragma pack(pop)// Restore previous packing alignment


static std::string aircraftInfoFilename{ "aircraft_info.yaml" };
static AircraftInfo aircraftInfo;


/**
 * Write the aircraft info to a YAML file.
 */
static bool writeAircraftInfo()
{
  std::ofstream ofs(aircraftInfoFilename);
  if (!ofs) {
    std::cerr << std::format(
      "[Failed to open file '{}' for writing, skipping AircraftInfo write]\n", aircraftInfoFilename);
    return false;
  }

  using namespace std::chrono;

  ofs.imbue(std::locale::classic());
  ofs << "kind: AircraftInfo\n"
      << "metadata:\n"
      << "  start-time: " << std::format("{:%FT%TZ}", floor<seconds>(system_clock::now())) << "\n"
      << "  simulator: \"MSFS2024\"\n"
      << "aircraft:\n"
      << "  title: \"" << aircraftInfo.title << "\"\n"
      << "  livery: \"" << aircraftInfo.livery << "\"\n"
      << "initial-position:\n"
      << std::setprecision(std::numeric_limits<double>::digits10)
      << "  latitude: " << aircraftInfo.planeLatitude << "\n"
      << "  longitude: " << aircraftInfo.planeLongitude << "\n"
      << "  altitude: " << aircraftInfo.planeAltitude << "\n"
      << std::setprecision(std::numeric_limits<float>::digits10)
      << "  pitch: " << aircraftInfo.planePitch << "\n"
      << "  bank: " << aircraftInfo.planeBank << "\n"
      << "  heading: " << aircraftInfo.planeHeading << "\n"
      << "  on-ground: " << ((aircraftInfo.onGround != 0) ? "true" : "false") << "\n"
      << "  air-speed: " << aircraftInfo.planeAirspeed << "\n";

  if (!ofs) {
    std::cerr << std::format("[Failed to write Aircraft info to '{}']\n", aircraftInfoFilename);
    return false;
  }
  std::cerr << std::format("[Aircraft info saved to '{}']\n", aircraftInfoFilename);
  return true;
}


static bool recordingActive{ false };
static int recordingSegment{ 0 };
static std::string positionDataFilenamePrefix{ "aircraft_position_" };
static std::string positionDataFilename;
static std::ofstream positionData;


/**
 * Start recording position data to the specified file.
 *
 * @param filename The name of the file to write position data to.
 * @return true if recording started successfully, false otherwise.
 */
static bool startPositionData(std::string filename)
{
  if (positionData.is_open()) { positionData.close(); }
  positionData.open(filename);
  positionData.imbue(std::locale::classic());
  positionData << "kind: AircraftPosition\n"
               << "metadata:\n"
               << "  start-time: "
               << std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()))
               << "\n"
               << "  simulator: \"MSFS2024\"\n"
               << "positions:\n";
  if (!positionData) {
    std::cerr << "[Failed to open 'aircraft_position.yaml' for writing, skipping position updates]\n";
    positionData.setstate(std::ios::badbit);
    return false;
  }

  HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect,
    REQID_AIRCRAFT_POSITION,
    DEFID_AIRCRAFT_POSITION,
    SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
    SIMCONNECT_PERIOD_SECOND,
    SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);
  if (FAILED(hr)) {
    std::cerr << std::format(
      "[Failed to request aircraft position data: HRESULT 0x{:08X}, skipping position updates]\n", hr);
    positionData.close();

    return false;
  }
  std::cerr << "[Position data recording started]\n";
  recordingActive = true;

  return true;
}


/**
 * Stop recording position data.
 */
static void stopPositionData()
{
  if (recordingActive) {
    if (positionData.is_open()) {
      positionData.close();
      std::cerr << "[Position data file closed]\n";
    }

    HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect,
      REQID_AIRCRAFT_POSITION,
      DEFID_AIRCRAFT_POSITION,
      SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
      SIMCONNECT_PERIOD_NEVER);
    if (FAILED(hr)) {
      std::cerr << std::format("[Failed to cancel aircraft position data request: HRESULT 0x{:08X}]\n", hr);
    }
    std::cerr << "[Position data stream stopped]\n";
    recordingActive = false;
  }
}


/**
 * Gather command-line arguments into the args map.
 *
 * All commandline arguments starting with '--' are treated as flags and key-value pairs.
 * The other arguments are treated as positional arguments with keys 'Arg0', 'Arg1', etc.
 * Entry "Arg0" is always the program name.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 */
static void gatherArgs(int argc, const char *argv[])
{
  args.clear();
  int fixedArg{ 0 };

  args["Arg" + std::to_string(fixedArg++)] = argv[0];
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.starts_with("--")) {
      auto eq = arg.find('=');
      if (eq != std::string::npos) {
        std::string key = arg.substr(2, eq - 2);
        std::string value = arg.substr(eq + 1);

        args[key] = value;
      } else {
        args[arg.substr(2)] = "";// No value provided
      }
    } else {
      args["Arg" + std::to_string(fixedArg++)] = arg;
    }
  }
}


auto main(int argc, const char *argv[]) -> int
{
  gatherArgs(argc, argv);

  std::chrono::seconds runDuration{ 0 };// Default to 0, meaning don't record position data
  if (args.contains("duration")) {
    try {
      runDuration = std::chrono::seconds(std::stoi(args["duration"]));
    } catch (const std::exception &) {
      std::cerr << std::format("[Invalid duration '{}', using default of 60 seconds]\n", args["duration"]);
      runDuration = std::chrono::seconds(60);
    }
  }

  // Gather filenames from arguments
  if (args.contains("info-filename")) { aircraftInfoFilename = args["info-filename"]; }
  if (args.contains("position-filename")) { positionDataFilename = args["position-filename"]; }
  if (args.contains("position-filename-prefix")) { positionDataFilenamePrefix = args["position-filename-prefix"]; }

  if ((runDuration.count() == 0) && !args.contains("keyboard")) {
    std::cerr << "[No duration specified and keyboard input not enabled. Use --duration=N or --keyboard]\n";
    return 1;
  }

  // Connect to the simulator
  if (!connect()) {
    std::cerr << "[ABORTING: Failed to connect to simulator]\n";
    return 1;
  }

  // Set up keyboard input if requested
  if (args.contains("keyboard") && !setupKeys()) {
    std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
    return 1;
  }

  // Define the data structures
  if (!defineAircraftInfo()) {
    disconnect();
    std::cerr << "[ABORTING: Failed to define aircraft info structure]\n";
    return 1;
  }
  if (!defineAircraftPosition()) {
    disconnect();
    std::cerr << "[ABORTING: Failed to define aircraft position structure]\n";
    return 1;
  }
  if (runDuration.count() > 0) {
    // We have a defined runlength, so assume a single recording session
    std::cerr << std::format("[Recording position data to '{}']\n", positionDataFilename);
    if (!startPositionData(positionDataFilename)) {
      disconnect();
      std::cerr << "[ABORTING: Failed to start position data recording]\n";
      return 1;
    }
  }

  // Request the aircraft info once
  HRESULT hr = SimConnect_RequestDataOnSimObject(
    hSimConnect, REQID_AIRCRAFT_INFO, DEFID_AIRCRAFT_INFO, SIMCONNECT_OBJECT_ID_USER_AIRCRAFT, SIMCONNECT_PERIOD_ONCE);
  if (FAILED(hr)) {
    disconnect();
    std::cerr << std::format("[ABORTING: Failed to request aircraft info: HRESULT 0x{:08X}]\n", hr);
    return 1;
  }

  handleMessages(runDuration);

  stopPositionData();

  disconnect();

  return 0;
}