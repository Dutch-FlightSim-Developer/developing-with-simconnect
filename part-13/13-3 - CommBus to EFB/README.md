# 13-3 - CommBus to EFB

This example shows what CommBus brings over ClientData (part-11): no shared memory area,
no data definition to set up in advance - just broadcast a named event with a string
payload, and anyone listening for that name receives it. It's split into two independent
pieces sharing one CommBus event name, `DutchFlightSim.Tutorial.Hello`:

- **[`C++/`](C++/)** - a small one-shot CLI tool, built through this repo's normal CMake
  pipeline. It joins its command-line arguments into a message and broadcasts it once via
  the abstraction library's new `Connection::callCommBusEvent()`.

- **[`EFB/`](EFB/)** - a minimal Electronic Flight Bag app that listens for the same event
  and displays the last message received. This is a different toolchain entirely: it's
  built with Node/npm/TypeScript and packaged/tested through MSFS Developer Mode's
  Project Editor, **not** through CMake. See [`EFB/README.md`](EFB/README.md) for setup.

## Trying it out

1. Build the C++ sender via your usual CMake preset (e.g. `windows-msvc-debug-user-mode-2024`).
2. Build and install the EFB app - see [`EFB/README.md`](EFB/README.md).
3. Start a flight in an aircraft with an EFB (the DA62 is a good default).
4. Open the "CommBus Message Viewer" app on the EFB.
5. From a terminal, run:

   ```
   13-3-CommBusToEFB-Sender.exe Hello from the terminal!
   ```

   The message should appear in the EFB app.

## Status

This is the first EFB/JS example in the repository. Confirmed working end-to-end:
running the C++ sender updates the message shown live in the EFB app. Getting there
surfaced a few EFB-specific gotchas not covered by the SDK's own `TemplateApp` sample -
see the "Status" section in [`EFB/README.md`](EFB/README.md) for details.
