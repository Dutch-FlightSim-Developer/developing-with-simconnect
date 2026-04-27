# Copilot Instructions

This repository provides a structured, educational walkthrough of using the Microsoft Flight Simulator **SimConnect API**, including both **raw C-style usage** and a **modern C++20 abstraction library**.

## Project Structure

- The root contains:
  - A **modern C++20 library** in `CppSimConnect/src/` and `include/simconnect/` that abstracts away all raw SimConnect usage.
  - **Training materials** organized into directories named `part-*`, each corresponding to a YouTube tutorial.
    - These contain subdirectories (e.g., `2-1`, `2-2`, etc.) showing individual examples, in order of presentation.

- Each example appears twice:
  - **"in C"** — raw SimConnect API (procedural style)
  - **"in C++"** — using the abstraction library (clean, idiomatic modern C++)

## Build System

### SDK Requirements
- Requires MSFS 2020 or 2024 SDK with environment variables:
  - `MSFS2020_SDK` or `MSFS_SDK` pointing to MSFS 2020 SDK root
  - `MSFS2024_SDK` pointing to MSFS 2024 SDK root
- Set SDK version via CMake: `-DMSFS_SDK_VERSION=2024` or `-DMSFS_SDK_VERSION=2020` (default: 2024)

### CMake Configuration
```bash
# Configure with a preset (recommended)
# All presets explicitly specify SDK version (2020 or 2024)

# User mode presets (clean builds):
cmake --preset windows-msvc-debug-user-mode-2024
cmake --preset windows-msvc-release-user-mode-2024
cmake --preset windows-clang-debug-2024
cmake --preset windows-clang-release-2024

# Developer mode presets (warnings-as-errors, debug only):
cmake --preset windows-msvc-debug-developer-mode-2024
cmake --preset windows-clang-debug-tests-2024

# Build
cmake --build out/build/<preset-name>
```

### Available Presets

All presets explicitly specify SDK version (`-2020` or `-2024`).

**MSVC Debug:**
- `windows-msvc-debug-developer-mode-2020/2024` — Warnings-as-errors (ASAN/clang-tidy disabled due to MSVC limitations)
- `windows-msvc-debug-user-mode-2020/2024` — Clean build, no extra tooling

**MSVC Release:**
- `windows-msvc-release-user-mode-2020/2024` — Release build, user mode only

**Clang:**
- `windows-clang-debug-2020/2024` — Debug build
- `windows-clang-release-2020/2024` — Release build
- `windows-clang-debug-tests-2020/2024` — Debug with tests
- `windows-clang-release-tests-2020/2024` — Release with tests

### Running Tests
```bash
# Run all tests
ctest --preset test-windows-msvc-debug-developer-mode

# Run specific test
ctest --preset <test-preset> -R <test-name>

# Build with tests enabled
cmake --preset windows-clang-debug-tests -DBUILD_TESTING=ON
```

### Test structure:
- **Unit tests**: `CppSimConnect/tests/` — Pure C++ library tests (no simulator required)
- **Live tests**: `CppSimConnect/live-tests/` — Integration tests requiring a running simulator

## SimConnect API references

### Location
- The Simconnect SDK uses the environment variable `MSFS_SDK` or `MSFS2020_SDK` for MSFS 2020, and `MSFS2024_SDK` for MSFS 2024.
- The SimConnect API headers are located in the `SimConnect SDK/include/` directory of the respective SDK.

### Restrictions
- Never suggest likely SimConnect API calls, types, or constants that do not actually exist in the SDK.


## Architecture

### Connection Hierarchy
- `Connection` — Abstract base for all SimConnect connections
  - `SimpleConnection` — Basic connection without event-driven messaging
  - `WindowsEventConnection` — Uses Windows Event objects for async message signaling (preferred)
  - `WindowsMessagingConnection` — Uses Windows message queue for MFC/Win32 apps

### Message Handler Hierarchy
- `SimConnectMessageHandler` — Base for all message dispatchers
  - `SimpleHandler` — Manual polling with lambda/function callbacks
  - `PollingHandler` — Polling-based dispatcher
  - `WindowsEventHandler` — Event-driven handler (pairs with `WindowsEventConnection`, preferred)

### Specialized Handlers (CRTP pattern)
- `MessageHandler<T>` — Base template for correlation-based handlers
  - `SystemStateHandler<T>` — Handles `RequestSystemState` responses
  - `SimObjectDataHandler<T>` — Handles `RequestDataOnSimObject` responses
  - `SystemEventHandler<T>` — Handles subscribed system events
  - `EventHandler<T>` — General event subscription handler

### Data Handling
- `DataBlock` — Base for data definition structures
  - `DataBlockReader` — Read-only view of received data
  - `DataBlockBuilder` — Builds data definitions for transmission
- `DataDefinitions` — Factory for creating and registering data definitions
- `Requests` — Utility for making SimConnect data requests

### Message Processing
- `HandlerProc` — Base for message processing strategies
  - `SimpleHandlerProc` — Single callback per message type
  - `MultiHandlerProc` — Multiple callbacks per message type

## Code Conventions

- C++20 features are encouraged:
  - Use `constexpr`, `enum class`, `std::variant`, `std::function`, and lambdas
  - Use templates and policy-based design instead of inheritance
  - RAII for all resource management (including SimConnect handles)
- Avoid raw pointers unless required for interop
- Avoid all manual memory management in C++ examples
- Prefer `std::string` over C-style strings
- Inline function definitions in headers are acceptable when justified
- Follow consistent naming:
  - PascalCase for types, camelCase for variables and functions
  - Explicit, descriptive names (e.g., `DataDefinition`, `SimObjectData`, not `foo` or `bar`)
  - IDs such as `definitionId`, `requestId`, etc. should reflect SimConnect terminology

## SimConnect-Specific Conventions

- Raw SimConnect API use is isolated to the C examples
- C++ examples rely entirely on the abstraction layer unless illustrating low-level concepts
- Message handling in C++ is abstracted using tagged types and dispatcher registration
- Data definitions are built using fluent, type-safe builders
- Asynchronous event handling uses safe, typed callbacks without blocking

## Avoid

- `Sleep()` or delays in the message loop
- Direct `switch` on `SIMCONNECT_RECV_ID_*` in user code — prefer dispatched handlers
- Mixing raw SimConnect calls with the abstraction layer
- Global state or static mutable variables
- Manual memory management or C-style allocation

## Best Practices

- C++ examples should prefer the abstraction layer for all SimConnect interactions
- The `WindowsEventConnection` is the preferred way to open and close connections
- The `WindowsEventHandler` is used to handle SimConnect messages in a type-safe manner

## Terminology

- **SimObject**: any simulator entity (e.g., user aircraft)
- **Request ID**: numerical ID for data/event requests
- **Definition ID**: numerical ID for data structure definitions
- **Data Definition**: a field layout for a data request
- **Dispatcher**: handles incoming SimConnect messages via tagged handlers

## Purpose

- Provide an upgrade path from procedural SimConnect usage to clean C++ APIs
- Serve as an educational companion to a structured YouTube tutorial series
- Enable developers to follow along using either low-level C or high-level C++
- Encourage best practices in modern C++ development
