/*
 * Copyright (c) 2026. Bert Laverman
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


#include <charconv>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/util/console_logger.hpp>

#include <simconnect/requests/input_event_handler.hpp>


using namespace SimConnect;
using namespace std::chrono_literals;


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
static void handleOpen(const Messages::OpenMsg &msg)
{
  std::cout << "Connected to " << &(msg.szApplicationName[0]) << " version "
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
static void handleClose([[maybe_unused]] const Messages::QuitMsg &msg) {
  std::cout << "Simulator shutting down.\n";
}


/**
 * Handle SimConnect Exception messages.
 *
 * @param msg The exception message to handle.
 */
[[maybe_unused]]
static void handleException(const Messages::ExceptionMsg &msg)
{
  std::cerr << std::format("Received an exception type {}:\n", msg.dwException);
  if (msg.dwSendID != unknownSendId) {
    std::cerr << std::format("- Related to a message with SendID {}.\n", msg.dwSendID);
  }
  if (msg.dwIndex != Exceptions::unknownIndex) { std::cerr << std::format("- Regarding parameter {}.\n", msg.dwIndex); }

  const ExceptionCode exc{ static_cast<ExceptionCode>(msg.dwException) };
  switch (exc) {
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
 * Gather command-line arguments into the args map.
 *
 * All commandline arguments starting with '--' are treated as flags and key-value pairs.
 * The other arguments are treated as positional arguments with keys 'Arg0', 'Arg1', etc.
 * Entry "Arg0" is always the program name.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line argument strings.
 * @return Map of gathered arguments.
 */
[[maybe_unused]]
static std::map<std::string, std::string> gatherArgs(int argc,
  const char *argv[]) // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
{
  std::map<std::string, std::string> args;
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
      } else {
        args[arg.substr(2)] = ""; // No value provided
      }
    } else {
      args["Arg" + std::to_string(fixedArg++)] = arg;
    }
  }
  return args;
}


/**
 * Split a raw ';'-separated parameter type string (see SimConnect_EnumerateInputEventParams)
 * into its individual, non-empty tokens.
 *
 * @param value The raw ';'-separated parameter type string.
 * @return The individual parameter type tokens, in order.
 */
static std::vector<std::string_view> splitParams(std::string_view value)
{
  std::vector<std::string_view> result;
  std::size_t start{ 0 };
  std::size_t end{ value.find(';') };
  while (end != std::string_view::npos) {
    const auto param{ value.substr(start, end - start) };
    if (!param.empty()) {
      result.push_back(param);
    }
    start = end + 1;
    end = value.find(';', start);
  }
  if (start < value.length()) {
    const auto param{ value.substr(start) };
    if (!param.empty()) {
      result.push_back(param);
    }
  }
  return result;
}


/**
 * A catalog-sourced client event: a name known ahead of time, either from a legacy MSFS SDK-doc
 * extraction or from a PMDG SDK header, sent via MapClientEventToSimEvent + TransmitClientEvent
 * rather than the input-event hash mechanism.
 */
struct CatalogEntry {
  std::string eventId;     ///< The SimConnect event-name string ("KEY_*" or PMDG's "#<number>").
  std::size_t paramCount;  ///< Number of parameters documented for this event.
};


/**
 * Skip whitespace starting at `pos`.
 *
 * @param text The JSON text.
 * @param pos The position to start at; updated to the first non-whitespace position.
 */
static void skipWhitespace(std::string_view text, std::size_t& pos)
{
  while (pos < text.size()) {
    const char ch{ text.at(pos) };
    if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') { break; }
    ++pos;
  }
}


/**
 * Check whether `text.at(pos)` is `expected`, without risking an out-of-range access.
 *
 * @param text The JSON text.
 * @param pos The position to check.
 * @param expected The character to compare against.
 * @return true if `pos` is in range and holds `expected`.
 */
static bool atChar(std::string_view text, std::size_t pos, char expected)
{
  return pos < text.size() && text.at(pos) == expected;
}


/**
 * If `text.at(pos)` is `expected`, consume it and advance `pos` past it.
 *
 * @param text The JSON text.
 * @param pos The position to check; advanced by one on success.
 * @param expected The character to consume.
 * @return true if `expected` was found and consumed, false otherwise (`pos` unchanged).
 */
static bool expectChar(std::string_view text, std::size_t& pos, char expected)
{
  if (!atChar(text, pos, expected)) { return false; }
  ++pos;
  return true;
}


/**
 * Consume a trailing ',' if present, skipping whitespace both before checking and (if consumed)
 * after it. Used between array elements / object members - leaves `pos` positioned either right
 * after a comma (more elements/members follow) or at the closing bracket/brace either way.
 *
 * @param text The JSON text.
 * @param pos The position to check; updated past any whitespace and consumed comma.
 */
static void skipCommaIfPresent(std::string_view text, std::size_t& pos)
{
  skipWhitespace(text, pos);
  if (expectChar(text, pos, ',')) { skipWhitespace(text, pos); }
}


/**
 * Parse exactly 4 hex digits (as in a \uXXXX escape) starting at `pos`, advancing past them.
 *
 * @param text The JSON text.
 * @param pos The position of the first hex digit; updated to just past the fourth on success.
 * @return The parsed 16-bit value, or std::nullopt if 4 valid hex digits are not present.
 */
static std::optional<unsigned int> parseHex4(std::string_view text, std::size_t& pos)
{
  if (pos + 4 > text.size()) { return std::nullopt; }
  unsigned int value{};
  const auto* const begin{ text.data() + pos }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto* const end{ begin + 4 }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto [ptr, ec] = std::from_chars(begin, end, value, 16);
  if (ec != std::errc{} || ptr != end) { return std::nullopt; }
  pos += 4;
  return value;
}


// UTF-8 encoding constants (RFC 3629): per-length max codepoint, lead-byte tag, and the shift/mask
// used to peel off each 6-bit continuation-byte group.
static constexpr unsigned int kUtf8OneByteMax{ 0x7FU };
static constexpr unsigned int kUtf8TwoByteMax{ 0x7FFU };
static constexpr unsigned int kUtf8ThreeByteMax{ 0xFFFFU };
static constexpr unsigned int kUtf8TwoByteTag{ 0xC0U };
static constexpr unsigned int kUtf8ThreeByteTag{ 0xE0U };
static constexpr unsigned int kUtf8FourByteTag{ 0xF0U };
static constexpr unsigned int kUtf8ContinuationTag{ 0x80U };
static constexpr unsigned int kUtf8ContinuationMask{ 0x3FU };
static constexpr unsigned int kUtf8ContinuationShift{ 6U };
static constexpr unsigned int kUtf8ThreeByteShift{ 12U };
static constexpr unsigned int kUtf8FourByteShift{ 18U };

// UTF-16 surrogate pair constants, for decoding a \uXXXX\uXXXX pair into a codepoint above the
// Basic Multilingual Plane (see parseUnicodeEscape).
static constexpr unsigned int kHighSurrogateMin{ 0xD800U };
static constexpr unsigned int kHighSurrogateMax{ 0xDBFFU };
static constexpr unsigned int kLowSurrogateMin{ 0xDC00U };
static constexpr unsigned int kLowSurrogateMax{ 0xDFFFU };
static constexpr unsigned int kSupplementaryPlaneStart{ 0x10000U };
static constexpr unsigned int kSurrogateShift{ 10U };


/**
 * Append the UTF-8 encoding of a Unicode codepoint to `out`.
 *
 * @param out The string to append the encoded bytes to.
 * @param codepoint The Unicode codepoint to encode (must be a valid scalar value, <= 0x10FFFF).
 */
static void appendUtf8(std::string& out, unsigned int codepoint)
{
  if (codepoint <= kUtf8OneByteMax) {
    out += static_cast<char>(codepoint);
  } else if (codepoint <= kUtf8TwoByteMax) {
    out += static_cast<char>(kUtf8TwoByteTag | (codepoint >> kUtf8ContinuationShift));
    out += static_cast<char>(kUtf8ContinuationTag | (codepoint & kUtf8ContinuationMask));
  } else if (codepoint <= kUtf8ThreeByteMax) {
    out += static_cast<char>(kUtf8ThreeByteTag | (codepoint >> kUtf8ThreeByteShift));
    out += static_cast<char>(kUtf8ContinuationTag | ((codepoint >> kUtf8ContinuationShift) & kUtf8ContinuationMask));
    out += static_cast<char>(kUtf8ContinuationTag | (codepoint & kUtf8ContinuationMask));
  } else {
    out += static_cast<char>(kUtf8FourByteTag | (codepoint >> kUtf8FourByteShift));
    out += static_cast<char>(kUtf8ContinuationTag | ((codepoint >> kUtf8ThreeByteShift) & kUtf8ContinuationMask));
    out += static_cast<char>(kUtf8ContinuationTag | ((codepoint >> kUtf8ContinuationShift) & kUtf8ContinuationMask));
    out += static_cast<char>(kUtf8ContinuationTag | (codepoint & kUtf8ContinuationMask));
  }
}


/**
 * Parse a "\uXXXX" escape (the "\u" already consumed - `pos` is at the first hex digit),
 * including a following low surrogate for a codepoint above the BMP, and append its UTF-8
 * encoding to `out`.
 *
 * @param text The JSON text.
 * @param pos The position of the first hex digit; updated to just past the escape(s) on success.
 * @param out The string to append the decoded character to.
 * @return true on success, false on an invalid or lone-surrogate escape.
 */
static bool parseUnicodeEscape(std::string_view text, std::size_t& pos, std::string& out)
{
  const auto high{ parseHex4(text, pos) };
  if (!high) { return false; }
  unsigned int codepoint{ *high };

  if (codepoint >= kLowSurrogateMin && codepoint <= kLowSurrogateMax) { return false; } // Lone low surrogate.
  if (codepoint >= kHighSurrogateMin && codepoint <= kHighSurrogateMax) {
    // High surrogate - must be followed by a low surrogate to form a full codepoint.
    if (!expectChar(text, pos, '\\') || !expectChar(text, pos, 'u')) { return false; }
    const auto low{ parseHex4(text, pos) };
    if (!low || *low < kLowSurrogateMin || *low > kLowSurrogateMax) { return false; }
    codepoint = kSupplementaryPlaneStart + ((codepoint - kHighSurrogateMin) << kSurrogateShift) + (*low - kLowSurrogateMin);
  }

  appendUtf8(out, codepoint);
  return true;
}


/**
 * Parse a JSON string literal starting at `text.at(pos)` (which must be '"'), unescaping the
 * standard JSON backslash escapes (including \uXXXX, re-encoded as UTF-8). Advances `pos` past
 * the closing quote.
 *
 * @param text The JSON text.
 * @param pos The position of the opening quote; updated to just past the closing quote.
 * @return The unescaped string content, or std::nullopt if malformed.
 */
static std::optional<std::string> parseString(std::string_view text, std::size_t& pos)
{
  if (!expectChar(text, pos, '"')) { return std::nullopt; }

  std::string result;
  while (pos < text.size() && text.at(pos) != '"') {
    if (text.at(pos) != '\\') {
      result += text.at(pos);
      ++pos;
      continue;
    }
    ++pos;
    if (pos >= text.size()) { return std::nullopt; }
    if (text.at(pos) == 'u') {
      ++pos;
      if (!parseUnicodeEscape(text, pos, result)) { return std::nullopt; }
      continue;
    }
    switch (text.at(pos)) {
    case '"': result += '"'; break;
    case '\\': result += '\\'; break;
    case '/': result += '/'; break;
    case 'b': result += '\b'; break;
    case 'f': result += '\f'; break;
    case 'n': result += '\n'; break;
    case 'r': result += '\r'; break;
    case 't': result += '\t'; break;
    default:
      return std::nullopt;
    }
    ++pos;
  }
  return expectChar(text, pos, '"') ? std::optional<std::string>(std::move(result)) : std::nullopt; // Unterminated if false.
}


/**
 * Parse an object member's key ("...":), consuming the trailing ':' and any surrounding
 * whitespace, leaving `pos` at the member value's first character.
 *
 * @param text The JSON text.
 * @param pos The position of the key's opening quote; updated to the value's first character.
 * @return The key, or std::nullopt if malformed.
 */
static std::optional<std::string> parseMemberKey(std::string_view text, std::size_t& pos)
{
  auto key{ parseString(text, pos) };
  if (!key) { return std::nullopt; }
  skipWhitespace(text, pos);
  if (!expectChar(text, pos, ':')) { return std::nullopt; }
  skipWhitespace(text, pos);
  return key;
}


/**
 * Skip one JSON value (string, object, array, number, true, false, or null) starting at `pos`,
 * without building any representation of it. Used to skip catalog fields this loader does not
 * need (source, category, description, and each params entry's own contents).
 *
 * @param text The JSON text.
 * @param pos The position of the value's first character; updated to just past the value.
 * @return true if a well-formed value was skipped, false otherwise.
 */
static bool skipValue(std::string_view text, std::size_t& pos)
{
  skipWhitespace(text, pos);
  if (pos >= text.size()) { return false; }

  if (atChar(text, pos, '"')) {
    return parseString(text, pos).has_value();
  }

  if (atChar(text, pos, '{') || atChar(text, pos, '[')) {
    const char open{ text.at(pos) };
    const char close{ open == '{' ? '}' : ']' };
    ++pos;
    skipWhitespace(text, pos);
    if (expectChar(text, pos, close)) { return true; } // Empty object/array.

    while (true) {
      if (open == '{' && !parseMemberKey(text, pos)) { return false; }
      if (!skipValue(text, pos)) { return false; } // Array element, or object member value.
      skipWhitespace(text, pos);
      if (expectChar(text, pos, ',')) { skipWhitespace(text, pos); continue; }
      return expectChar(text, pos, close);
    }
  }

  // number, true, false, or null - skip until a structural character or whitespace.
  const std::size_t start{ pos };
  while (pos < text.size()) {
    const char ch{ text.at(pos) };
    if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') { break; }
    ++pos;
  }
  return pos > start;
}


/**
 * Count the elements of a JSON array starting at `text.at(pos)` (which must be '['), by skipping
 * each element in turn. Advances `pos` past the closing bracket.
 *
 * @param text The JSON text.
 * @param pos The position of the opening bracket; updated to just past the closing bracket.
 * @return The element count, or std::nullopt if malformed.
 */
static std::optional<std::size_t> countArrayElements(std::string_view text, std::size_t& pos)
{
  if (!expectChar(text, pos, '[')) { return std::nullopt; }
  skipWhitespace(text, pos);
  if (expectChar(text, pos, ']')) { return 0; }

  std::size_t count{ 0 };
  while (true) {
    if (!skipValue(text, pos)) { return std::nullopt; }
    ++count;
    skipWhitespace(text, pos);
    if (expectChar(text, pos, ',')) { skipWhitespace(text, pos); continue; }
    return expectChar(text, pos, ']') ? std::optional<std::size_t>(count) : std::nullopt;
  }
}


/**
 * Apply one member of an event object to the in-progress parse state: captures "name"/"eventId"
 * strings and the "params" array's element count; any other member is skipped.
 *
 * @param text The JSON text.
 * @param pos The position of the member's value; updated to just past it.
 * @param key The member's key.
 * @param name Updated if `key` is "name".
 * @param eventId Updated if `key` is "eventId".
 * @param paramCount Updated if `key` is "params".
 * @return true if the member's value was consumed successfully, false on malformed JSON.
 */
static bool applyEventField(std::string_view text, std::size_t& pos, std::string_view key,
  std::optional<std::string>& name, std::optional<std::string>& eventId, std::size_t& paramCount)
{
  if (key == "name") {
    name = parseString(text, pos);
    return name.has_value();
  }
  if (key == "eventId") {
    eventId = parseString(text, pos);
    return eventId.has_value();
  }
  if (key == "params") {
    const auto count{ countArrayElements(text, pos) };
    if (!count) { return false; }
    paramCount = *count;
    return true;
  }
  return skipValue(text, pos);
}


/**
 * Parse one event object from the catalog's "events" array, inserting it into `catalog` if it
 * has both a "name" and an "eventId" string. Other members (params, description, ...) are
 * consumed but only "params" is used, for its element count.
 *
 * @param text The JSON text.
 * @param pos The position of the object's opening brace; updated to just past its closing brace.
 * @param catalog The name -> entry map to insert the parsed event into.
 * @return true if a well-formed object was consumed (whether or not it had enough fields to be
 *         inserted), false on malformed JSON.
 */
static bool parseEvent(std::string_view text, std::size_t& pos, std::unordered_map<std::string, CatalogEntry>& catalog)
{
  if (!expectChar(text, pos, '{')) { return false; }
  skipWhitespace(text, pos);

  std::optional<std::string> name;
  std::optional<std::string> eventId;
  std::size_t paramCount{ 0 };

  while (pos < text.size() && !atChar(text, pos, '}')) {
    const auto key{ parseMemberKey(text, pos) };
    if (!key || !applyEventField(text, pos, *key, name, eventId, paramCount)) { return false; }
    skipCommaIfPresent(text, pos);
  }
  if (!expectChar(text, pos, '}')) { return false; }

  if (name && eventId) {
    catalog.insert_or_assign(*name, CatalogEntry{ .eventId = *eventId, .paramCount = paramCount });
  }
  return true;
}


/**
 * Parse the catalog's "events" array, merging each well-formed element into `catalog`.
 *
 * @param text The JSON text.
 * @param pos The position of the array's opening bracket; updated to just past its closing bracket.
 * @param catalog The name -> entry map to merge parsed events into.
 * @return true if the array was well-formed, false otherwise.
 */
static bool parseEventsArray(std::string_view text, std::size_t& pos, std::unordered_map<std::string, CatalogEntry>& catalog)
{
  if (!expectChar(text, pos, '[')) { return false; }
  skipWhitespace(text, pos);
  while (pos < text.size() && !atChar(text, pos, ']')) {
    if (!parseEvent(text, pos, catalog)) { return false; }
    skipCommaIfPresent(text, pos);
  }
  return expectChar(text, pos, ']');
}


/**
 * Parse the top-level object of an msfs-events-schema catalog document, extracting just the
 * fields this loader needs: each event's "name", "eventId", and its "params" array's element
 * count. A hand-rolled scan rather than a general JSON library - see the 14-4 issue discussion:
 * the MSFS-SDK-bundled rapidjson fails to compile under this project's strict Clang settings
 * (a genuine upstream bug, not fixable from example code), and the schema here is entirely our
 * own (cleanup.py, scripts/extract_msfs_events.py), so a small purpose-built scanner is simpler
 * than pulling in a different JSON dependency.
 *
 * @param text The full JSON document text.
 * @param catalog The name -> entry map to merge parsed events into.
 * @return true if the document was well-formed enough to scan, false otherwise.
 */
static bool parseCatalog(std::string_view text, std::unordered_map<std::string, CatalogEntry>& catalog)
{
  std::size_t pos{ 0 };
  skipWhitespace(text, pos);
  if (!expectChar(text, pos, '{')) { return false; }
  skipWhitespace(text, pos);

  while (pos < text.size() && !atChar(text, pos, '}')) {
    const auto key{ parseMemberKey(text, pos) };
    if (!key) { return false; }

    const bool ok{ (*key == "events") ? parseEventsArray(text, pos, catalog) : skipValue(text, pos) };
    if (!ok) { return false; }

    skipCommaIfPresent(text, pos);
  }
  return atChar(text, pos, '}');
}


/**
 * Read one msfs-events-schema JSON catalog file and merge its events into `catalog`. Malformed
 * or unreadable files are skipped with a warning rather than aborting the whole load.
 *
 * @param path The catalog file to parse.
 * @param catalog The name -> entry map to merge parsed events into.
 */
static void loadCatalogFile(const std::filesystem::path& path, std::unordered_map<std::string, CatalogEntry>& catalog)
{
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cerr << std::format("[Failed to open catalog file '{}']\n", path.string());
    return;
  }
  const std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

  if (!parseCatalog(content, catalog)) {
    std::cerr << std::format("[Skipping malformed catalog file '{}']\n", path.string());
  }
}


/**
 * Load every *.json catalog file in `dir` (non-recursive) into a single name -> entry map. A
 * missing directory is a normal case (catalogs are gitignored, generated locally per SDK year)
 * rather than an error - it just means the map comes back empty.
 *
 * @param dir The catalog directory to scan - typically the active SDK year's msfs-events folder.
 * @return The merged name -> entry map.
 */
static std::unordered_map<std::string, CatalogEntry> loadCatalogs(const std::filesystem::path& dir)
{
  std::unordered_map<std::string, CatalogEntry> catalog;

  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    std::cerr << std::format("[No catalog directory at '{}' - skipping.]\n", dir.string());
    return catalog;
  }

  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      loadCatalogFile(entry.path(), catalog);
    }
  }
  std::cerr << std::format("[Loaded {} catalog event(s) from '{}']\n", catalog.size(), dir.string());
  return catalog;
}


/**
 * Parse a decimal or "0x"-prefixed hexadecimal unsigned value, as used for a client event's data
 * parameter.
 *
 * @param text The value text, as given on the command line.
 * @return The parsed value, or std::nullopt if `text` is not a valid number in either base.
 */
static std::optional<unsigned long> parseUnsignedValue(std::string_view text)
{
  if (text.empty()) { return std::nullopt; }

  const bool isHex{ text.starts_with("0x") || text.starts_with("0X") };
  const auto digits{ isHex ? text.substr(2) : text };
  if (digits.empty()) { return std::nullopt; }

  unsigned long value{};
  const auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, isHex ? 16 : 10); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return (ec == std::errc{} && ptr == digits.data() + digits.size()) ? std::optional<unsigned long>(value) : std::nullopt; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}


/**
 * Look the input event up, get its current value, and print it.
 *
 * @param handler The message handler to pump while waiting for the value to arrive.
 * @param inputEvents The input event handler to fetch the value through.
 * @param event The event to fetch the value of.
 * @param done Set to true once the fetch completes (by this function) or an exception arrived
 *             (by the caller's exception handler) - either way, pumping should stop.
 * @return 0 on success, 1 otherwise.
 */
template <class Handler>
static int getValue(Handler& handler, InputEventHandler<Handler>& inputEvents, const InputEvent& event, bool& done)
{
  int exitCode{ 1 };

  if (event.type == InputEventTypes::doubleValue) {
    inputEvents.getInputEvent(event.hash, [&done, &exitCode, name = event.name](double value) {
      std::cout << std::format("Current value of '{}': {}\n", name, value);
      exitCode = 0;
      done = true;
    });
  }
  else {
    inputEvents.getInputEvent(event.hash, [&done, &exitCode, name = event.name](const std::string& value) {
      std::cout << std::format("Current value of '{}': '{}'\n", name, value);
      exitCode = 0;
      done = true;
    });
  }

  static constexpr auto timeout = 10s;
  handler.handleUntilOrTimeout([&done]() { return done; }, timeout);

  return exitCode;
}


/**
 * Parse `newValue` and set the input event to it. Only supported for input events with a single
 * FLOAT64 parameter - anything else is rejected with a clear message rather than guessed at.
 *
 * @param handler The message handler to pump while giving a possible failure exception a
 *                grace window to arrive.
 * @param connection The connection to send the raw SetInputEvent call through.
 * @param rawParams The event's raw ';'-separated parameter type string, to validate against.
 * @param event The event to set the value of.
 * @param newValue The new value, as given on the command line.
 * @param done Set to true if an exception arrived (by the caller's exception handler) - checked
 *             after the grace window to tell a failure from an assumed success.
 * @return 0 on success, 1 otherwise (unsupported shape, bad value, exception).
 */
template <class Handler, class Connection>
static int setValue(Handler& handler, Connection& connection, std::string_view rawParams, const InputEvent& event, std::string_view newValue, bool& done)
{
  const auto params = splitParams(rawParams);
  if (event.type != InputEventTypes::doubleValue || params.size() != 1 || params.front() != "FLOAT64") {
    std::cerr << std::format(
      "'{}' has an unsupported parameter shape for set (only a single FLOAT64 parameter is currently supported).\n", event.name);
    return 1;
  }

  double parsedValue{};
  const auto [ptr, ec] = std::from_chars(newValue.data(), newValue.data() + newValue.size(), parsedValue); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  if (ec != std::errc{}) {
    std::cerr << std::format("'{}' is not a valid number.\n", newValue);
    return 1;
  }

  const std::span<const std::byte> bytes{ reinterpret_cast<const std::byte*>(&parsedValue), sizeof(parsedValue) }; //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  connection.setInputEvent(event.hash, bytes);

  // SetInputEvent generates no success response - only exceptions tell us it failed. Give any
  // exception a short grace window to arrive before assuming it worked.
  static constexpr auto setGrace = 2s;
  handler.handleUntilOrTimeout([&done]() { return done; }, setGrace);
  if (done) {
    return 1; // The exception handler already printed why.
  }

  std::cout << std::format("Set '{}' to {}.\n", event.name, parsedValue);
  return 0;
}


/**
 * Map and transmit a classic client event - covers both legacy MSFS "KEY_*" events and PMDG's
 * "#<number>" third-party events. A leading "KEY_" is stripped before mapping: that prefix is the
 * legacy gauge-header C-macro naming convention (e.g. `#define KEY_FLAPS_1 ...`), not part of the
 * actual SimConnect event-name string - MapClientEventToSimEvent wants the bare name ("FLAPS_1"),
 * confirmed against 8-2's proven raw-C example and live-tested (a "KEY_"-prefixed name gets
 * rejected with SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED). PMDG's "#<number>" strings never start
 * with "KEY_", so the strip is a no-op for them.
 *
 * @param handler The message handler to pump while giving a possible failure exception a
 *                grace window to arrive.
 * @param connection The connection to map and transmit the event through.
 * @param eventIdString The SimConnect event-name string ("KEY_*" or "#<number>").
 * @param data The single data value to send with the event.
 * @param done Set to true if an exception arrived (by the caller's exception handler) - checked
 *             after the grace window to tell a failure from an assumed success.
 * @return 0 on success, 1 otherwise (exception).
 */
template <class Handler, class Connection>
static int sendClientEvent(Handler& handler, Connection& connection, std::string_view eventIdString, unsigned long data, bool& done)
{
  std::string_view mapName{ eventIdString };
  if (mapName.starts_with("KEY_")) {
    static constexpr std::size_t keyPrefixLength{ 4 };
    mapName.remove_prefix(keyPrefixLength);
  }

  auto evt = connection.event(mapName);
  connection.mapClientEvent(evt);
  connection.transmitClientEventWithPriority(SimObject::userAircraft, evt, Events::standardPriority, data);

  // TransmitClientEvent generates no success response - only exceptions tell us it failed. Give
  // any exception a short grace window to arrive before assuming it worked.
  static constexpr auto sendGrace = 2s;
  handler.handleUntilOrTimeout([&done]() { return done; }, sendGrace);
  if (done) {
    return 1; // The exception handler already printed why.
  }

  std::cout << std::format("Sent client event '{}' with value {}.\n", eventIdString, data);
  return 0;
}


/**
 * Connect to the simulator and look `name` up: first as a declared input event on the current
 * aircraft (get/set, as in 14-3), then - if not declared - as a classic client event, either from
 * a loaded JSON catalog or (with no catalog entry) as a literal client-event id string.
 *
 * @param name The exact (case-sensitive) name, or literal client-event id, to look up.
 * @param newValue The value to set/send, or std::nullopt to get (input event) / just report what
 *                 is known (client event) instead.
 * @param catalogDir The directory of *.json catalog files to search when `name` is not a
 *                   declared input event.
 * @return 0 on success, 1 otherwise (not found, connection failure, unsupported shape, bad value).
 */
static int runTest(std::string_view name, std::optional<std::string_view> newValue, const std::filesystem::path& catalogDir)
{
  WindowsEventConnection<false, ConsoleLogger> connection;
  WindowsEventHandler<false, ConsoleLogger> handler(connection);
  handler.autoClosing(true);

  bool done{ false };

  handler.registerDefaultHandler([](const Messages::MsgBase& msg) {
    std::cerr << std::format("Ignoring message of type {} (length {} bytes)\n", msg.dwID, msg.dwSize);
  });
  handler.registerHandler<Messages::OpenMsg>(Messages::open, handleOpen);
  handler.registerHandler<Messages::QuitMsg>(Messages::quit, handleClose);
  handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, [&done](const Messages::ExceptionMsg& msg) {
    handleException(msg);
    done = true;
  });

  if (!connection.open()) {
    std::cerr << "Failed to connect to simulator.\n";
    return 1;
  }

  InputEventHandler<decltype(handler)> inputEvents(handler);

  // First: is this a declared input event on the current aircraft? (A zero-declared-events
  // aircraft raises an exception here rather than an empty result - the exception handler above
  // sets `done`, and we fall through to the catalog below exactly as if nothing had been found.)
  std::cout << std::format("[Looking up input event '{}']\n", name);

  bool foundInputEvent{ false };
  InputEvent event{};
  std::string rawParams;

  auto lookupRequest = inputEvents.findInputEvent(name,
    [&foundInputEvent, &event, &rawParams, &done](const InputEvent& foundEvent, std::string_view params) {
      foundInputEvent = true;
      event = foundEvent;
      rawParams = std::string(params);
      done = true;
    },
    [&done]() { done = true; });

  static constexpr auto timeout = 10s;
  handler.handleUntilOrTimeout([&done]() { return done; }, timeout);
  lookupRequest.stop();

  if (foundInputEvent) {
    done = false;
    return newValue ? setValue(handler, connection, rawParams, event, *newValue, done)
                     : getValue(handler, inputEvents, event, done);
  }

  // Not a declared input event - fall back to the client-event catalog (year-scoped), or treat
  // the typed name itself as a literal client-event id if no catalog entry matches.
  const auto catalog{ loadCatalogs(catalogDir) };
  const auto found{ catalog.find(std::string(name)) };
  const bool haveCatalogEntry{ found != catalog.end() };

  if (!newValue) {
    if (haveCatalogEntry) {
      std::cout << std::format(
        "'{}' is a client event (id '{}', {} parameter(s) documented). Provide a value to send it.\n",
        name, found->second.eventId, found->second.paramCount);
    } else {
      std::cout << std::format(
        "'{}' is not a declared input event or a known catalog entry - provide a value to send it as a literal "
        "client-event id.\n", name);
    }
    return 0;
  }

  if (haveCatalogEntry && found->second.paramCount > 1) {
    std::cerr << std::format(
      "'{}' takes {} parameters - only single-parameter client events are currently supported.\n",
      name, found->second.paramCount);
    return 1;
  }

  const auto data{ parseUnsignedValue(*newValue) };
  if (!data) {
    std::cerr << std::format("'{}' is not a valid numeric value.\n", *newValue);
    return 1;
  }

  const std::string eventIdString{ haveCatalogEntry ? found->second.eventId : std::string(name) };

  done = false;
  return sendClientEvent(handler, connection, eventIdString, *data, done);
}


/**
 * Walk upward from `start` looking for a directory containing ".git", to locate the repo root
 * regardless of how deep the build output nests the executable - CMake/ninja
 * (out/build/<preset>/part-14/14-4.../) and MSBuild (part-14/14-4.../x64/<config>/) nest it to
 * different depths, so a fixed number of ".." segments can't cover both.
 *
 * @param start The directory to start searching from (typically the executable's own directory).
 * @return The repo root, or std::nullopt if no ".git" marker was found within the search bound.
 */
static std::optional<std::filesystem::path> findRepoRoot(const std::filesystem::path& start)
{
  std::error_code ec;
  auto dir{ std::filesystem::absolute(start, ec) };
  if (ec) { return std::nullopt; }

  static constexpr int maxLevels{ 8 };
  for (int level{ 0 }; level < maxLevels; ++level) {
    std::error_code existsEc;
    if (std::filesystem::exists(dir / ".git", existsEc)) { return dir; }
    const auto parent{ dir.parent_path() };
    if (parent == dir) { break; } // Reached filesystem root.
    dir = parent;
  }
  return std::nullopt;
}


auto main(int argc, const char* argv[]) -> int // NOLINT(bugprone-exception-escape)
{
  const auto args{ gatherArgs(argc, argv) };
  const auto nameArg{ args.find("Arg1") };
  if (nameArg == args.end() || nameArg->second.empty()) {
    std::cerr << std::format("Usage: {} <event-name> [value] [--catalog-dir=<path>]\n", args.at("Arg0"));
    return 1;
  }
  const std::string name{ nameArg->second };

  const auto valueArg{ args.find("Arg2") };
  const std::optional<std::string_view> newValue{
    (valueArg != args.end()) ? std::optional<std::string_view>(valueArg->second) : std::nullopt
  };

  static constexpr std::string_view catalogYear{
#if MSFS_2024_SDK
    "2024"
#else
    "2020"
#endif
  };
  const auto repoRoot{ findRepoRoot(std::filesystem::path(args.at("Arg0")).parent_path()) };
  std::filesystem::path catalogDir{
    repoRoot ? (*repoRoot / "msfs-events" / catalogYear) : (std::filesystem::path{ "msfs-events" } / catalogYear)
  };
  const auto catalogDirArg{ args.find("catalog-dir") };
  if (catalogDirArg != args.end() && !catalogDirArg->second.empty()) {
    catalogDir = catalogDirArg->second;
  }

  try {
    return runTest(name, newValue, catalogDir);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
