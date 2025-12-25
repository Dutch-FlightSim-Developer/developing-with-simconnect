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

#include <exception>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <locale>
#include <format>
#include <string>
#include <string_view>
#include <array>
#include <map>
#include <utility>
#include <functional>

#include <cstdint>
#include <limits>
#include <chrono>


#include <simconnect/simconnect.hpp>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/input_group.hpp>

#include <simconnect/data_definition.hpp>
#include <simconnect/data_frequency.hpp>
#include <simconnect/requests/request.hpp>
#include <simconnect/requests/simobject_data_handler.hpp>

#include <simconnect/util/logger.hpp>
#include <simconnect/util/console_logger.hpp>

using namespace SimConnect;
using namespace std::chrono_literals;


/**
 * Return a pretty formatted version string.
 * @param major major version number. If 0, return "Unknown".
 * @param minor minor version number. If 0, return just the major version number.
 * @return version string.
 */
static std::string version(unsigned long major, unsigned long minor) {
	if (major == 0) {
		return "Unknown";
	}
	return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


/**
 * Handle the SIMCONNECT_RECV_OPEN message.
 */
static void handleOpen(const Messages::OpenMsg& msg) {
	std::cout << "Connected to " << &(msg.szApplicationName[0])
		<< " version " << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << '\n'
		<< "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << '\n'
		<< "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor) << '\n'
		<< "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << '\n';
}


/**
 * Handle the SIMCONNECT_RECV_QUIT message.
 */
static void handleClose([[maybe_unused]] const Messages::QuitMsg& msg) {
	std::cout << "Simulator shutting down.\n";
}

/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
static void handleException(const Messages::ExceptionMsg& msg)
{

    std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
    if (msg.dwSendID != unknownSendId)
    {
        std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
    }
    if (msg.dwIndex != Exceptions::unknownIndex)
    {
        std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex);
    }

    const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
    switch (exc)
    {
    case Exceptions::none: // Should never happen
        std::cerr << "No exception.\n";
        break;
    case Exceptions::error:
        std::cerr << "Some unspecific error has occurred.\n";
        break;
    case Exceptions::sizeMismatch:
        std::cerr << "The size of the parameter does not match the expected size.\n";
        break;
    case Exceptions::unrecognizedId:
        std::cerr << "The parameter is not a recognized ID.\n";
        break;
    case Exceptions::unopened:
        std::cerr << "The connection has not been opened.\n";
        break;
    case Exceptions::versionMismatch:
        std::cerr << "This version of SimConnect cannot work with this version of the simulator.\n";
        break;
    case Exceptions::tooManyGroups:
        std::cerr << "The maximum number of (input/notification) groups has been reached. (currently 20)\n";
        break;
    case Exceptions::nameUnrecognized:
        std::cerr << "The parameter is not a recognized name.\n";
        break;
    case Exceptions::tooManyEventNames:
        std::cerr << "The maximum number of event names has been reached. (currently 1000)\n";
        break;
    case Exceptions::eventIdDuplicate:
        std::cerr << "The event ID is already in use.\n";
        break;
    case Exceptions::tooManyMaps:
        std::cerr << "The maximum number of mapings has been reached. (currently 20)\n";
        break;
    case Exceptions::tooManyObjects:
        std::cerr << "The maximum number of objects has been reached. (currently 1000)\n";
        break;
    case Exceptions::tooManyRequests:
        std::cerr << "The maximum number of requests has been reached. (currently 1000)\n";
        break;
    case Exceptions::weatherInvalidPort: // Legacy
        std::cerr << "The weather port is invalid.\n";
        break;
    case Exceptions::weatherInvalidMetar: // Legacy
        std::cerr << "The METAR string is invalid.\n";
        break;
    case Exceptions::weatherUnableToGetObservation: // Legacy
        std::cerr << "Unable to get the observation.\n";
        break;
    case Exceptions::weatherUnableToCreateStation: // Legacy
        std::cerr << "Unable to create the station.\n";
        break;
    case Exceptions::weatherUnableToRemoveStation: // Legacy
        std::cerr << "Unable to remove the station.\n";
        break;
    case Exceptions::invalidDataType:
        std::cerr << "The requested data cannot be converted to the specified data type.\n";
        break;
    case Exceptions::invalidDataSize:
        std::cerr << "The requested data cannot be transferred in the specified data size.\n";
        break;
    case Exceptions::dataError:
        std::cerr << "The data passed is invalid.\n";
        break;
    case Exceptions::invalidArray:
        std::cerr << "The array passed to SetDataOnSimObject is invalid.\n";
        break;
    case Exceptions::createObjectFailed:
        std::cerr << "The AI object could not be created.\n";
        break;
    case Exceptions::loadFlightplanFailed:
        std::cerr << "The flight plan could not be loaded. Either it could not be found, or it contained an error.\n";
        break;
    case Exceptions::operationInvalidForObjectType:
        std::cerr << "The operation is not valid for the object type.\n";
        break;
    case Exceptions::illegalOperation:
        std::cerr << "The operation is illegal. (AI or Weather)\n";
        break;
    case Exceptions::alreadySubscribed:
        std::cerr << "The client is already subscribed to this event.\n";
        break;
    case Exceptions::invalidEnum:
        std::cerr << "The type enum value is unknown. (Probably an unknown type in RequestDataOnSimObjectType)\n";
        break;
    case Exceptions::definitionError:
        std::cerr << "The definition is invalid. (Probably a variable length requested in RequestDataOnSimObject)\n";
        break;
    case Exceptions::duplicateId:
        std::cerr << "The ID is already in use. (Menu, DataDefinition item ID, ClientData mapping, or event to notification group)\n";
        break;
    case Exceptions::datumId:
        std::cerr << "Unknown datum ID specified for SetDataOnSimObject.\n";
        break;
    case Exceptions::outOfBounds:
        std::cerr << "The requested value is out of bounds. (radius of a RequestDataOnSimObjectType, or CreateClientData)\n";
        break;
    case Exceptions::alreadyCreated:
        std::cerr << "A ClientData area with that name has already been created.\n";
        break;
    case Exceptions::objectOutsideRealityBubble:
        std::cerr << "The AI object is outside the reality bubble.\n";
        break;
    case Exceptions::objectContainer:
        std::cerr << "The AI object creation failed. (container issue)\n";
        break;
    case Exceptions::objectAi:
        std::cerr << "The AI object creation failed. (AI issue)\n";
        break;
    case Exceptions::objectAtc:
        std::cerr << "The AI object creation failed. (ATC issue)\n";
        break;
    case Exceptions::objectSchedule:
        std::cerr << "The AI object creation failed. (scheduling issue)\n";
        break;
    case Exceptions::jetwayData:
        std::cerr << "Requesting JetWay data failed.\n";
        break;
    case Exceptions::actionNotFound:
        std::cerr << "The action was not found.\n";
        break;
    case Exceptions::notAnAction:
        std::cerr << "The action was not a valid action.\n";
        break;
    case Exceptions::incorrectActionParams:
        std::cerr << "The action parameters were incorrect.\n";
        break;
    case Exceptions::getInputEventFailed:
        std::cerr << "The input event name was not found. (GetInputEvent)\n";
        break;
    case Exceptions::setInputEventFailed:
        std::cerr << "The input event name was not found. (SetInputEvent)\n";
        break;
    case Exceptions::internal:
        break;
    default:
        std::cerr << std::format("An unknown exception code was received: {}.\n", msg.dwException);
        break;
    }
}


inline constexpr unsigned titleSize{ 128 };
inline constexpr unsigned liverySize{ 256 };

#pragma pack(push, 1) // Ensure no padding bytes are added to the structure
struct AircraftInfo {
    std::array<char, titleSize> title{};    // TITLE (String, max 128 chars)
	std::array<char, liverySize> livery{};  // LIVERY NAME (String, max 128 chars)
    double planeLatitude{ 0.0 };            // PLANE ALTITUDE (Degrees)
    double planeLongitude{ 0.0 };           // PLANE LONGITUDE (Degrees)
    double planeAltitude{ 0.0 };            // PLANE ALTITUDE (Feet)

    float planePitch{ 0.0F };               // PLANE PITCH DEGREES (Degrees)
    float planeBank{ 0.0F };                // PLANE BANK DEGREES (Degrees)
    float planeHeading{ 0.0F };             // PLANE HEADING DEGREES TRUE (Degrees)

	int32_t onGround{ 0 };                  // SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)

    float planeAirspeed{ 0.0F };            // AIRSPEED TRUE (Knots)
};
#pragma pack(pop) // Restore previous packing alignment


inline constexpr float littleBitPrecise{ 0.1F };
inline constexpr float morePrecise{ 0.01F };
inline constexpr float veryPrecise{ 0.0001F };


/**
 * Write the aircraft info to a YAML file.
 */
static bool writeAircraftInfo(const AircraftInfo& aircraftInfo, const std::string& aircraftInfoFilename) {
    std::ofstream ofs(aircraftInfoFilename);
    if (!ofs) {
        std::cerr << std::format("[Failed to open file '{}' for writing, skipping AircraftInfo write]\n", aircraftInfoFilename);
        return false;
    }

    using namespace std::chrono;

	ofs.imbue(std::locale::classic());
    ofs << "kind: AircraftInfo\n"
        << "metadata:\n"
        << "  start-time: " << std::format("{:%FT%TZ}", floor<seconds>(system_clock::now())) << "\n"
        << "  simulator: \"MSFS2024\"\n"
        << "aircraft:\n"
        << "  title: \"" << std::string_view{ aircraftInfo.title.data(), aircraftInfo.title.size() } << "\"\n"
        << "  livery: \"" << std::string_view{ aircraftInfo.livery.data(), aircraftInfo.livery.size() } << "\"\n"
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


#pragma pack(push, 1) // Ensure no padding bytes are added to the structure
struct AircraftPosition {
    double planeLatitude;           // PLANE ALTITUDE (Degrees)
	double planeLongitude;          // PLANE LONGITUDE (Degrees)
	double planeAltitude;           // PLANE ALTITUDE (Feet)

    float planePitch;               // PLANE PITCH DEGREES (Degrees)
	float planeBank;                // PLANE BANK DEGREES (Degrees)
	float planeHeading;             // PLANE HEADING DEGREES TRUE (Degrees)
	
    float planeAirspeed;            // AIRSPEED INDICATED (Knots)

    float planeVelocityX;           // VELOCITY BODY X (Feet per second)
	float planeVelocityY;           // VELOCITY BODY Y (Feet per second)
	float planeVelocityZ;           // VELOCITY BODY Z (Feet per second)

    float planeAccelerationX;       // ACCELERATION BODY X (Feet per second squared)
	float planeAccelerationY;       // ACCELERATION BODY Y (Feet per second squared)
	float planeAccelerationZ;       // ACCELERATION BODY Z (Feet per second squared)

    float planeRotationVelocityX;   // ROTATION VELOCITY BODY X (Degrees per second)
	float planeRotationVelocityY;   // ROTATION VELOCITY BODY Y (Degrees per second)
	float planeRotationVelocityZ;   // ROTATION VELOCITY BODY Z (Degrees per second)

    int32_t onGround;               // SIM ON GROUND / SIM SHOULD SET ON GROUND (Bool, defaulting to 32-bit int 0 or 1)
};
#pragma pack(pop) // Restore previous packing alignment


class PositionDataWriter {
    DataDefinition<AircraftPosition> aircraftPosition;

    bool segmented{ false };
    bool recordingActive{ false };
    int recordingSegment{ 0 };
    std::string positionDataFilenamePrefix{ "aircraft_position_" };
    std::string positionDataFilename;
    std::ofstream positionData;

    Request dataRequest_;

public:
    PositionDataWriter(std::string filename, bool segmentedFiles)
        : segmented{ segmentedFiles },
          positionDataFilename{ std::move(filename) }
    {
        defineAircraftPosition();
    }
    PositionDataWriter(const PositionDataWriter&) = delete;
    PositionDataWriter(PositionDataWriter&&) = delete;
    PositionDataWriter& operator=(const PositionDataWriter&) = delete;
    PositionDataWriter & operator=(PositionDataWriter&&) = delete;

    ~PositionDataWriter() = default;


    // Accessors

    [[nodiscard]]
    bool isSegmented() const noexcept {
        return segmented;
    }
    void setSegmented(bool segmentedFiles) noexcept {
        segmented = segmentedFiles;
    }

    [[nodiscard]]
    bool isRecordingActive() const noexcept {
        return recordingActive;
    }
    void setRecordingActive(bool active) noexcept {
        recordingActive = active;
    }

    void setPositionDataFilename(std::string_view filename) {
        positionDataFilename = filename;
    }
    void setPositionDataFilenamePrefix(std::string_view filenamePrefix) {
        positionDataFilenamePrefix = filenamePrefix;
    }

    /**
     * Build a filename if needed.
     */
    void makeFilename()
    {
        if (segmented) {
            positionDataFilename = std::format("{}{:03}.yaml", positionDataFilenamePrefix, recordingSegment++);
        }
    }


    /**
     * Define the data structure for the aircraft position.
     *
     * @param def The data definition to define the aircraft position on.
     */
    void defineAircraftPosition() {
        aircraftPosition
            .addFloat64(&AircraftPosition::planeLatitude, "PLANE LATITUDE", "degrees", veryPrecise)
            .addFloat64(&AircraftPosition::planeLongitude, "PLANE LONGITUDE", "degrees", veryPrecise)
            .addFloat64(&AircraftPosition::planeAltitude, "PLANE ALTITUDE", "feet", veryPrecise)
            .addFloat32(&AircraftPosition::planePitch, "PLANE PITCH DEGREES", "degrees", veryPrecise)
            .addFloat32(&AircraftPosition::planeBank, "PLANE BANK DEGREES", "degrees", veryPrecise)
            .addFloat32(&AircraftPosition::planeHeading, "PLANE HEADING DEGREES TRUE", "degrees", veryPrecise)
            .addFloat32(&AircraftPosition::planeAirspeed, "AIRSPEED TRUE", "knots", littleBitPrecise)
            .addFloat32(&AircraftPosition::planeVelocityX, "VELOCITY BODY X", "feet per second", morePrecise)
            .addFloat32(&AircraftPosition::planeVelocityY, "VELOCITY BODY Y", "feet per second", morePrecise)
            .addFloat32(&AircraftPosition::planeVelocityZ, "VELOCITY BODY Z", "feet per second", morePrecise)
            .addFloat32(&AircraftPosition::planeAccelerationX, "ACCELERATION BODY X", "feet per second squared", morePrecise)
            .addFloat32(&AircraftPosition::planeAccelerationY, "ACCELERATION BODY Y", "feet per second squared", morePrecise)
            .addFloat32(&AircraftPosition::planeAccelerationZ, "ACCELERATION BODY Z", "feet per second squared", morePrecise)
            .addFloat32(&AircraftPosition::planeRotationVelocityX, "ROTATION VELOCITY BODY X", "degrees per second", veryPrecise)
            .addFloat32(&AircraftPosition::planeRotationVelocityY, "ROTATION VELOCITY BODY Y", "degrees per second", veryPrecise)
            .addFloat32(&AircraftPosition::planeRotationVelocityZ, "ROTATION VELOCITY BODY Z", "degrees per second", veryPrecise)
            .addInt32(&AircraftPosition::onGround, "SIM ON GROUND", "bool");
    }


    /**
     * Start recording position data to the specified file.
     *
     * @return true if recording started successfully, false otherwise.
     */
    template <typename DataHandler>
    bool startPositionData(DataHandler& dataHandler)
    {
        if (positionData.is_open()) {
            positionData.close();
        }
        makeFilename();
        positionData.open(positionDataFilename);
        positionData.imbue(std::locale::classic());
        positionData
            << "kind: AircraftPosition\n"
            << "metadata:\n"
            << "  start-time: " << std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())) << "\n"
            << "  simulator: \"MSFS2024\"\n"
            << "positions:\n";
        if (!positionData) {
            std::cerr << "[Failed to open 'aircraft_position.yaml' for writing, skipping position updates]\n";
            positionData.setstate(std::ios::badbit);
            return false;
        }

        dataRequest_ = dataHandler.template requestData<AircraftPosition>(aircraftPosition, [&](const AircraftPosition& pos){ write(pos); },
            DataFrequency::every().second(), PeriodLimits::none(), SimObject::userCurrent, onlyWhenChanged);
        std::cerr << "[Position data recording started]\n";
        recordingActive = true;

        return true;
    }


    /**
     * Stop recording position data.
     */
    void stopPositionData()
    {
        if (recordingActive) {
            if (positionData.is_open()) {
                positionData.close();
                std::cerr << "[Position data file closed]\n";
            }

            dataRequest_.stop();
            dataRequest_ = Request{};
            std::cerr << "[Position data stream stopped]\n";
            recordingActive = false;
        }
    }


    /**
     * Toggle recording position data.
     */
    template <typename DataHandler>
    void toggleRecording(DataHandler& dataHandler)
    {
        if (recordingActive) {
            stopPositionData();
        } else {
            startPositionData(dataHandler);
        }
    }


    /**
     * Write the aircraft position to the YAML file.
     *
     * @param pos The aircraft position to write.
     */
    void write(const AircraftPosition& pos)
    {
        if (!positionData) {
            return;
        }
        positionData
            << "- msecs: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() << "\n"
            << std::setprecision(std::numeric_limits<double>::digits10)
            << "  latitude: " << pos.planeLatitude << "\n"
            << "  longitude: " << pos.planeLongitude << "\n"
            << "  altitude: " << pos.planeAltitude << "\n"
            << std::setprecision(std::numeric_limits<float>::digits10)
            << "  pitch: " << pos.planePitch << "\n"
            << "  bank: " << pos.planeBank << "\n"
            << "  heading: " << pos.planeHeading << "\n"
            << "  airspeed: " << pos.planeAirspeed << "\n"
            << "  velocity-x: " << pos.planeVelocityX << "\n"
            << "  velocity-y: " << pos.planeVelocityY << "\n"
            << "  velocity-z: " << pos.planeVelocityZ << "\n"
            << "  acceleration-x: " << pos.planeAccelerationX << "\n"
            << "  acceleration-y: " << pos.planeAccelerationY << "\n"
            << "  acceleration-z: " << pos.planeAccelerationZ << "\n"
            << "  rotation-velocity-x: " << pos.planeRotationVelocityX << "\n"
            << "  rotation-velocity-y: " << pos.planeRotationVelocityY << "\n"
            << "  rotation-velocity-z: " << pos.planeRotationVelocityZ << "\n"
            << "  on-ground: " << ((pos.onGround != 0) ? "true" : "false") << "\n";
    }
};


/**
 * Set up keyboard input to toggle recording and exit the program.
 */
template <typename EvtHandler>
static bool setupKeys(EvtHandler& eventHandler, std::function<void()> onToggleRecording, std::function<void()> onExit) // NOLINT(bugprone-easily-swappable-parameters)
{
    std::cerr
        << "[Press the Play/Pause media key to toggle recording]\n"
        << "[Press the Stop key to exit the program]\n";

    InputGroup inputGroup = eventHandler.createInputGroup().withHighestPriority();

    const event startStop = event::get("Toggle.Recording");
    inputGroup.addEvent(startStop, "VK_MEDIA_PLAY_PAUSE");
    eventHandler.template registerEventHandler<Messages::EventMsg>(startStop, [&onToggleRecording]([[maybe_unused]] const Messages::EventMsg& evt) {
        onToggleRecording();
    });

    const event exit = event::get("Exit.Program");
    inputGroup.addEvent(exit, "VK_MEDIA_STOP");
    eventHandler.template registerEventHandler<Messages::EventMsg>(exit, [&onExit]([[maybe_unused]] const Messages::EventMsg& evt) {
        onExit();
    });

    return inputGroup.enable();
}


/**
 * Define the data structure for the aircraft info.
 *
 * @param def The data definition to define the aircraft info on.
 */
static void defineAircraftInfo(DataDefinition<AircraftInfo>& def) {
    def.add(&AircraftInfo::title, DataTypes::string128, "TITLE")
         .add(&AircraftInfo::livery, DataTypes::string256, "LIVERY NAME")
         .addFloat64(&AircraftInfo::planeLatitude, "PLANE LATITUDE", "degrees", veryPrecise)
         .addFloat64(&AircraftInfo::planeLongitude, "PLANE LONGITUDE", "degrees", veryPrecise)
         .addFloat64(&AircraftInfo::planeAltitude, "PLANE ALTITUDE", "feet", veryPrecise)
         .addFloat32(&AircraftInfo::planePitch, "PLANE PITCH DEGREES", "degrees", veryPrecise)
         .addFloat32(&AircraftInfo::planeBank, "PLANE BANK DEGREES", "degrees", veryPrecise)
         .addFloat32(&AircraftInfo::planeHeading, "PLANE HEADING DEGREES TRUE", "degrees", veryPrecise)
         .addInt32(&AircraftInfo::onGround, "SIM ON GROUND", "bool")
         .addFloat32(&AircraftInfo::planeAirspeed, "AIRSPEED TRUE", "knots", littleBitPrecise);
}


/**
 * Load aircraft info from the simulator.
 *
 * @param dataHandler The data handler to use for requesting data.
 * @param maxDuration The maximum duration to wait for the aircraft info.
 * @param args The command-line arguments map.
 * @return true if aircraft info was successfully loaded, false otherwise.
 */
template <typename DataHandler>
static bool loadAircraftInfo(DataHandler& dataHandler, std::chrono::milliseconds maxDuration, std::map<std::string, std::string>& args)
{
    const std::string aircraftInfoFilename{ args.contains("info-filename") ? args["info-filename"] : "aircraft_info.yaml" };

    DataDefinition<AircraftInfo> aircraftInfoDef;
    defineAircraftInfo(aircraftInfoDef);

    bool haveAircraftInfo{ false };

    auto infoRequest = dataHandler.template requestDataOnce<AircraftInfo>(aircraftInfoDef, [&aircraftInfoFilename, &haveAircraftInfo](const AircraftInfo& info) {
        std::cerr << std::format("[Received aircraft info: '{}', livery '{}']\n", std::string_view(info.title.data(), info.title.size()), std::string_view(info.livery.data(), info.livery.size()));
        // aircraftInfo = info;
        haveAircraftInfo = true;
        if (!writeAircraftInfo(info, aircraftInfoFilename)) {
            std::cerr << "[Failed to write aircraft info. Aborting.]\n";
            haveAircraftInfo = false;
        }
    });
    dataHandler.simConnectMessageHandler().handleUntil([&haveAircraftInfo]() { return haveAircraftInfo; }, maxDuration);

    return haveAircraftInfo;
}


/**
 * Gather command-line arguments into the args map.
 * 
 * All commandline arguments starting with '--' are treated as flags and key-value pairs.
 * The other arguments are treated as positional arguments with keys 'Arg0', 'Arg1', etc.
 * Entry "Arg0" is always the program name.
 * 
 * @param args The map to store the gathered arguments.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 */
static void gatherArgs(std::map<std::string, std::string>& args, int argc, const char* argv[]) // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
{
    args.clear();
    int fixedArg{ 0 };

    args["Arg" + std::to_string(fixedArg++)] = argv[0]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (arg.starts_with("--")) {
            auto eqPos = arg.find('=');
            if (eqPos != std::string::npos) {
                const std::string key = arg.substr(2, eqPos - 2);
                const std::string value = arg.substr(eqPos + 1);

                args[key] = value;
            }
            else {
                args[arg.substr(2)] = ""; // No value provided
            }
        }
        else {
            args["Arg" + std::to_string(fixedArg++)] = arg;
        }
    }
}


auto main(int argc, const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{

    static constexpr const char* appName = "List parkings";

    std::map<std::string, std::string> args;

	gatherArgs(args, argc, argv);

    std::chrono::seconds runDuration{ 0 }; // Default to 0, meaning don't record position data
    constexpr auto defaultDuration{ 60s };

    if (args.contains("duration")) {
        try {
            runDuration = std::chrono::seconds(std::stoi(args["duration"]));
        }
        catch (const std::exception&) {
            std::cerr << std::format("[Invalid duration '{}', using default of 60 seconds]\n", args["duration"]);
            runDuration = defaultDuration;
        }
	}

    if ((runDuration.count() == 0) && !args.contains("keyboard")) {
        std::cerr << "[No duration specified and keyboard input not enabled. Use --duration=N or --keyboard]\n";
        return 1;
	}

    // Connect to the simulator
    WindowsEventConnection<true, ConsoleLogger> connection(appName);
    connection.logger().level(LogLevel::Debug);
    WindowsEventHandler<true, ConsoleLogger> connectionHandler(connection);
    EventHandler<WindowsEventHandler<true, ConsoleLogger>> eventHandler(connectionHandler);

    connectionHandler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
	connectionHandler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    connectionHandler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (!connection.open()) {
        std::cerr << "[ABORTING: Failed to connect to the simulator]\n";
        return 1;
    }

    SimObjectDataHandler dataHandler(connectionHandler);


    constexpr auto maxDuration = 5s;
    if (!loadAircraftInfo(dataHandler, maxDuration, args)) {
        std::cerr << "[ABORTING: Did not receive aircraft info in time]\n";
        return 1;
    }

    PositionDataWriter positionDataWriter(args.contains("position-filename") ? args["position-filename"] : "aircraft_position.yaml", args.contains("segment-files"));
	// Gather filenames from arguments
    if (args.contains("position-filename")) {
        positionDataWriter.setPositionDataFilename(args["position-filename"]);
    }
    if (args.contains("position-filename-prefix")) {
        positionDataWriter.setPositionDataFilenamePrefix(args["position-filename-prefix"]);
    }

    // Set up keyboard input if requested
    if (args.contains("keyboard") && !setupKeys(eventHandler,
        [&positionDataWriter, &dataHandler]() {
            std::cerr << "[Toggle recording requested from keyboard input]\n";
            positionDataWriter.toggleRecording(dataHandler);
        },
        [&connection, &positionDataWriter]() {
            std::cerr << "[Exit requested from keyboard input]\n";
            positionDataWriter.stopPositionData();
            connection.close();
        }))
    {
		std::cerr << "[ABORTING: Failed to set up keyboard input]\n";
        return 1;
    }

    if (runDuration.count() > 0) {
		// We have a defined runlength, so assume a single recording session
        positionDataWriter.startPositionData(dataHandler);
    }


	connectionHandler.handle(runDuration);

    positionDataWriter.stopPositionData();
    connection.close();

    return 0;
}