# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Purpose

Educational C++20 codebase paired with a YouTube tutorial series demonstrating Microsoft Flight Simulator SimConnect API usage. Each tutorial example appears twice: once in raw C (procedural SimConnect), once in C++ using the abstraction library in `include/simconnect/`.

## Build System

### Prerequisites

- MSFS SDK environment variables:
  - MSFS 2024: `MSFS2024_SDK` pointing to the SDK root
  - MSFS 2020: `MSFS2020_SDK` or `MSFS_SDK`
- CMake 3.21+, Ninja, MSVC or Clang

### Configure and Build

```bash
# Configure (choose a preset)
cmake --preset windows-msvc-debug-developer-mode-2024   # strict warnings, no ASAN/clang-tidy
cmake --preset windows-msvc-debug-user-mode-2024        # clean debug build
cmake --preset windows-msvc-release-user-mode-2024      # release
cmake --preset windows-clang-debug-tests-2024           # clang + unit/live tests enabled

# Build
cmake --build out/build/<preset-name>
```

All presets explicitly encode the SDK version (`-2020` or `-2024`). Default SDK version is 2024. Override with `-DMSFS_SDK_VERSION=2020`.

### Tests

Unit tests (no simulator required) are in `CppSimConnect/tests/`. Live tests (requires running MSFS) are in `CppSimConnect/live-tests/`.

Tests are only built with `-tests` presets or when `BUILD_TESTING=ON`.

```bash
# Run all tests
ctest --preset test-windows-msvc-debug-developer-mode

# Run a specific test by name
ctest --preset <test-preset> -R <test-name>
```

## Architecture

### Connection Hierarchy (CRTP + policy-based templates)

`Connection<Derived, ThreadSafe, Logger, TrackMappedEvents>` is the abstract base:
- `SimpleConnection` — basic polling, no event-driven messaging
- `WindowsEventConnection` — preferred; uses Windows Event objects for async signaling
- `WindowsMessagingConnection` — for MFC/Win32 apps using Windows message queue

### Message Handler Hierarchy

`SimConnectMessageHandler` dispatches incoming messages. Concrete types:
- `SimpleHandler` — manual polling with callbacks
- `PollingHandler` — polling with timeout
- `WindowsEventHandler` — event-driven, pairs with `WindowsEventConnection` (preferred)

### Specialized Handlers (CRTP, correlation-based)

`MessageHandler<T>` is the base for handlers that correlate responses to requests by ID:
- `SystemStateHandler<T>` — `RequestSystemState` responses
- `SimObjectDataHandler<T>` — `RequestDataOnSimObject` responses
- `SystemEventHandler<T>` — subscribed system events
- `EventHandler<T>` — general event subscriptions
- `ClientDataHandler<T>` — client data messages

### Data Handling

- `DataBlock` / `DataBlockReader` / `DataBlockBuilder` — read/write typed data structures for SimConnect fields
- `DataDefinitions` — factory to create and register data definitions with SimConnect
- `ClientDataDefinition` — tagged client data (e.g., for PMDG integration)
- `Requests` — utility to make SimConnect data requests

### Message Processing Policies

- `SimpleHandlerProc` — single callback per message type
- `MultiHandlerPolicy<T>` — multiple callbacks per message type

## Code Conventions

- **C++20 throughout**: `constexpr`, `enum class`, `std::variant`, lambdas, templates, RAII
- **Naming**: PascalCase for types; camelCase for variables and functions; IDs mirror SimConnect terminology (`definitionId`, `requestId`)
- **No raw pointers** unless required for interop; no manual memory management in C++ code
- **Headers are the library**: most logic lives in `include/simconnect/`; inline definitions in headers are acceptable

## SimConnect-Specific Rules

- Raw SimConnect API calls belong only in the C examples (`part-N/N-1 ... in C/`)
- C++ examples use the abstraction layer exclusively
- Never switch directly on `SIMCONNECT_RECV_ID_*`; use the typed dispatcher registrations
- Never suggest SimConnect API calls, types, or constants that do not actually exist in the SDK — check the SDK headers under `$MSFS2024_SDK/SimConnect SDK/include/`
- Do not use `Sleep()` or any blocking delay in a message loop

## Formatting and Linting

- **clang-format**: 120-column limit, 2-space indent, right-aligned pointers, sorted includes
- **clang-tidy**: enabled on Clang presets; disabled on MSVC presets (MSVC limitations); configuration in `.clang-tidy`
- Run clang-tidy via Clang debug presets; MSVC developer-mode presets use warnings-as-errors instead

## Dependencies (CPM-managed, auto-downloaded)

- `fmtlib/fmt` 11.1.4 — string formatting
- `spdlog` 1.15.2 — logging
- `googletest` v1.15.2 — testing
- `CLI11` 2.5.0 — command-line parsing

Optional: PMDG 737 NG3 SDK — auto-detected from `LocalAppData` or via `-DPMDG_NG3_SDK_PATH`.
