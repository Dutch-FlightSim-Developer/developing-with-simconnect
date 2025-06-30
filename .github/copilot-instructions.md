# Copilot Instructions

This repository provides a structured, educational walkthrough of using the Microsoft Flight Simulator **SimConnect API**, including both **raw C-style usage** and a **modern C++20 abstraction library**.

## Project Structure

- The root contains:
  - A **modern C++20 library** in `src/` and `include/` that abstracts away all raw SimConnect usage.
  - **Training materials** organized into directories named `part-*`, each corresponding to a YouTube tutorial.
    - These contain subdirectories (e.g., `2-1`, `2-2`, etc.) showing individual examples, in order of presentation.

- Each example appears twice:
  - **"in C"** — raw SimConnect API (procedural style)
  - **"in C++"** — using the abstraction library (clean, idiomatic modern C++)

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
