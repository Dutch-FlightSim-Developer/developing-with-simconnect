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


// PMDG 737 NG3 - CDU Screen Monitor (C++ version)
//
// Renders one CDU screen (left or right) in the console using ANSI escape
// sequences for cursor positioning and color, updated on every ON_SET delivery.
//
// Usage:
//   cdu_screen [0|l|left]      left CDU  (default)
//   cdu_screen  1|r|right      right CDU
//
// Prerequisites:
//   - MSFS running with the PMDG 737 loaded.
//   - In 737NG3_Options.ini, [SDK] section:
//       EnableDataBroadcast=1
//       EnableCDUBroadcast.0=1      (for left CDU)
//       EnableCDUBroadcast.1=1      (for right CDU)
//
// Color mapping (ANSI nearest match):
//   WHITE   -> bright white   CYAN -> bright cyan   GREEN -> bright green
//   MAGENTA -> bright magenta AMBER -> dark yellow   RED   -> bright red
//
// Flag rendering:
//   SMALL_FONT -> lowercase alpha characters (label/header rows have no font-size in a console)
//   REVERSE    -> ANSI reverse video
//   UNUSED     -> ANSI dim

#pragma warning(push, 3)
#include <Windows.h>
#include "PMDG_NG3_SDK.h"
#pragma warning(pop)

#include <cctype>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/requests/client_data_handler.hpp>
#include <simconnect/data/client_data_definition.hpp>
#include <simconnect/data_frequency.hpp>

#include <simconnect/util/console_logger.hpp>


using namespace SimConnect;


// Console layout: title on row 1, CDU starts at row 2, col 2 (1-based).
static constexpr int CONSOLE_ROW_OFFSET{ 2 };
static constexpr int CONSOLE_COL_OFFSET{ 2 };

// ANSI escape sequences
static constexpr std::string_view ANSI_RESET       { "\033[0m"    };
static constexpr std::string_view ANSI_BG_BLACK    { "\033[40m"   };
static constexpr std::string_view ANSI_DIM         { "\033[2m"    };
static constexpr std::string_view ANSI_REVERSE     { "\033[7m"    };
static constexpr std::string_view ANSI_CURSOR_HIDE { "\033[?25l"  };
static constexpr std::string_view ANSI_CURSOR_SHOW { "\033[?25h"  };
static constexpr std::string_view ANSI_CLEAR       { "\033[2J"    };

// Foreground color sequences indexed by PMDG_NG3_CDU_COLOR_* values
static constexpr const char* CDU_FG_COLORS[] = {
    "\033[97m",   // 0 WHITE   — bright white
    "\033[96m",   // 1 CYAN    — bright cyan
    "\033[92m",   // 2 GREEN   — bright green
    "\033[95m",   // 3 MAGENTA — bright magenta
    "\033[33m",   // 4 AMBER   — dark yellow (closest ANSI match)
    "\033[91m",   // 5 RED     — bright red
};
static constexpr int NUM_CDU_COLORS{ static_cast<int>(std::size(CDU_FG_COLORS)) };


static void enableAnsiVT()
{
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode{ 0 };
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

static void moveTo(int row, int col)
{
    std::cout << std::format("\033[{};{}H", row, col);
}

static void printTitle(int cduIndex)
{
    moveTo(1, 1);
    std::cout << ANSI_RESET
              << std::format("  PMDG 737  CDU {}  (Ctrl+C to quit)",
                             cduIndex == 0 ? "LEFT " : "RIGHT");
}

static void renderScreen(const PMDG_NG3_CDU_Screen& screen)
{
    std::cout << ANSI_CURSOR_HIDE;

    if (!screen.Powered) {
        // Clear the CDU area and show an off indicator.
        for (int row = 0; row < CDU_ROWS; ++row) {
            moveTo(CONSOLE_ROW_OFFSET + row, CONSOLE_COL_OFFSET);
            std::cout << ANSI_RESET << ANSI_BG_BLACK
                      << std::string(CDU_COLUMNS, ' ') << ANSI_RESET;
        }
        moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS / 2, CONSOLE_COL_OFFSET + 7);
        std::cout << "\033[90m[ CDU OFF ]" << ANSI_RESET;
    }
    else {
        // Cells[col][row] — iterate row-major for output.
        for (int row = 0; row < CDU_ROWS; ++row) {
            moveTo(CONSOLE_ROW_OFFSET + row, CONSOLE_COL_OFFSET);
            std::cout << ANSI_BG_BLACK;

            for (int col = 0; col < CDU_COLUMNS; ++col) {
                const PMDG_NG3_CDU_Cell& cell = screen.Cells[col][row];

                // Attributes: reset first, then apply flags and color.
                std::cout << ANSI_RESET << ANSI_BG_BLACK;

                if (cell.Flags & PMDG_NG3_CDU_FLAG_REVERSE) {
                    std::cout << ANSI_REVERSE;
                }

                const int colorIdx = (cell.Color < NUM_CDU_COLORS) ? cell.Color : 0;
                std::cout << CDU_FG_COLORS[colorIdx];

                if (cell.Flags & PMDG_NG3_CDU_FLAG_UNUSED) {
                    std::cout << ANSI_DIM;
                }

                // Printable ASCII range only; everything else is a space.
                char sym = (cell.Symbol >= 0x20 && cell.Symbol < 0x7F)
                    ? static_cast<char>(cell.Symbol) : ' ';

                // SMALL_FONT marks label/header rows — render as lowercase alpha
                // as a visual hint, since we can't change the font size.
                if ((cell.Flags & PMDG_NG3_CDU_FLAG_SMALL_FONT) && std::isalpha(static_cast<unsigned char>(sym))) {
                    sym = static_cast<char>(std::tolower(static_cast<unsigned char>(sym)));
                }

                std::cout << sym;
            }
            std::cout << ANSI_RESET;
        }
    }

    // Park the cursor below the CDU area and flush.
    moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS + 1, 1);
    std::cout << ANSI_CURSOR_SHOW;
    std::cout.flush();
}


/**
 * Return a pretty formatted version string.
 *
 * @param major Major version number. If 0, return "Unknown".
 * @param minor Minor version number. If 0, return just the major version number.
 * @return version string.
 */
static std::string version(unsigned long major, unsigned long minor)
{
    if (major == 0) { return "Unknown"; }
    return (minor == 0) ? std::to_string(major) : std::format("{}.{}", major, minor);
}


/**
 * Handle the SIMCONNECT_RECV_OPEN message.
 */
[[maybe_unused]]
static void handleOpen(const Messages::OpenMsg& msg)
{
    // Use stderr to avoid corrupting the ANSI screen layout.
    std::cerr << "Connected to " << &(msg.szApplicationName[0]) << " version "
              << version(msg.dwApplicationVersionMajor, msg.dwApplicationVersionMinor) << '\n'
              << "  build " << version(msg.dwApplicationBuildMajor, msg.dwApplicationBuildMinor) << '\n'
              << "  using SimConnect version " << version(msg.dwSimConnectVersionMajor, msg.dwSimConnectVersionMinor)
              << '\n'
              << "  build " << version(msg.dwSimConnectBuildMajor, msg.dwSimConnectBuildMinor) << '\n';
}


/**
 * Handle the SIMCONNECT_RECV_QUIT message.
 */
[[maybe_unused]]
static void handleClose([[maybe_unused]] const Messages::QuitMsg& msg) { std::cerr << "Simulator shutting down.\n"; }


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
[[maybe_unused]]
static void handleException(const Messages::ExceptionMsg& msg)
{
    std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
    if (msg.dwSendID != unknownSendId) {
        std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
    }
    if (msg.dwIndex != Exceptions::unknownIndex) { std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex); }

    const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
    switch (exc) {
    case Exceptions::none:
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
    case Exceptions::weatherInvalidPort:
        std::cerr << "The weather port is invalid.\n";
        break;
    case Exceptions::weatherInvalidMetar:
        std::cerr << "The METAR string is invalid.\n";
        break;
    case Exceptions::weatherUnableToGetObservation:
        std::cerr << "Unable to get the observation.\n";
        break;
    case Exceptions::weatherUnableToCreateStation:
        std::cerr << "Unable to create the station.\n";
        break;
    case Exceptions::weatherUnableToRemoveStation:
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
        std::cerr << "The ID is already in use. (Menu, DataDefinition item ID, ClientData mapping, or event to "
                     "notification group)\n";
        break;
    case Exceptions::datumId:
        std::cerr << "Unknown datum ID specified for SetDataOnSimObject.\n";
        break;
    case Exceptions::outOfBounds:
        std::cerr
            << "The requested value is out of bounds. (radius of a RequestDataOnSimObjectType, or CreateClientData)\n";
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
#if MSFS_2024_SDK
    case Exceptions::internal:
        std::cerr << "An internal SimConnect error has occurred.\n";
        break;
#endif
    default:
        std::cerr << std::format("An unknown exception code was received: {}.\n", msg.dwException);
        break;
    }
}


/**
 * Parse command-line arguments and return the CDU index (0=left, 1=right).
 * Returns -1 on invalid input.
 */
static int gatherArgs(int argc, const char* argv[])
{
    if (argc >= 2) {
        const std::string arg{ argv[1] };
        if (arg == "1" || arg == "r" || arg == "right") {
            return 1;
        }
        if (arg == "0" || arg == "l" || arg == "left") {
            return 0;
        }
        std::cerr << std::format("Usage: {} [0|l|left | 1|r|right]\n", argv[0]);
        return -1;
    }
    return 0; // default: left CDU
}


void runTest(int cduIndex)
{
    enableAnsiVT();
    std::cout << ANSI_CLEAR << ANSI_BG_BLACK;
    printTitle(cduIndex);

    WindowsEventConnection<false, ConsoleLogger> connection;
    WindowsEventHandler<false, ConsoleLogger> handler(connection);
    ClientDataHandler<WindowsEventHandler<false, ConsoleLogger>> dataHandler(handler);
    handler.autoClosing(true);
    //connection.logger().level(LogLevel::Trace);

    handler.registerDefaultHandler([](const Messages::MsgBase& msg) {
        std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
    });
    handler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
    handler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
    handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);

    if (connection.open()) {
        const char* const cduName = (cduIndex == 0) ? PMDG_NG3_CDU_0_NAME : PMDG_NG3_CDU_1_NAME;

        // PMDG data areas are identified by name only — the numeric IDs in the SDK
        // header are irrelevant. Let the handler allocate its own ID.
        ClientDataId cduId = dataHandler.mapClientDataName(cduName);

        ClientDataDefinition<PMDG_NG3_CDU_Screen> cduDef;
        cduDef.addRaw();

        auto cduReq = dataHandler.requestClientData<PMDG_NG3_CDU_Screen>(
            cduId, cduDef,
            renderScreen,
            ClientDataFrequency::onSet(),
            PeriodLimits::none(),
            /*onlyWhenChanged=*/ true);

        handler.handleUntilClosed();
    } else {
        std::cerr << "Failed to connect to simulator.\n";
    }

    // Restore terminal state on exit.
    moveTo(CONSOLE_ROW_OFFSET + CDU_ROWS + 2, 1);
    std::cout << ANSI_RESET << ANSI_CURSOR_SHOW;
    std::cout.flush();
}


auto main(int argc, const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
    try {
        const int cduIndex = gatherArgs(argc, argv);
        if (cduIndex < 0) { return 1; }
        runTest(cduIndex);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
