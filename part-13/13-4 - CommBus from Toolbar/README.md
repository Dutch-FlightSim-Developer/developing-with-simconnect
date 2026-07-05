# 13-4 - CommBus from Toolbar

The mirror image of 13-3: instead of a C++ app broadcasting and an EFB app listening, this
example is a plain MSFS toolbar panel that *sends* a CommBus event, with a C++ app that
listens.

- **[`Panel/`](Panel/)** - a minimal in-game panel (built and tested through MSFS Developer
  Mode's Project Editor, **not** through CMake - same toolchain family as 13-3's `EFB/`,
  but this is a plain `InGamePanels` package, not an EFB app). Clicking its toolbar icon
  opens a small window with a text field and a checkbox. Clicking "Send" packages both into
  JSON and broadcasts it via `CommBusListener::callSimConnect()`.

- **[`C++/`](C++/)** - a listener built on the abstraction library's `CommBusHandler` (see
  `include/simconnect/comm_bus_handler.hpp`), which reassembles any multi-chunk broadcasts
  and hands the listener a single `string_view`. Parses the JSON payload itself with a small
  hand-rolled extractor (see the note below on why, not a JSON library).

## Event contract

- Event name: `DutchFlightSim.Tutorial.HelloJson`
- Payload: a JSON string, e.g. `{"message":"Hello from the toolbar!","flag":true}`
  - `message` (string) - the text field's contents
  - `flag` (boolean) - the checkbox's checked state

This is a different event name from 13-3's `DutchFlightSim.Tutorial.Hello` (plain string
payload), since the payload shape differs.

## Building and testing

1. Enable Developer Mode in MSFS 2024 (see the SDK docs if you haven't already).
2. **File > Open project...** and select `Panel/CommBusFromToolbarProject.xml`.
3. In the Project Editor, click **Build All In Project**. Do this *before* starting a
   flight - packages aren't hot-reloaded into an already-running toolbar. A full MSFS
   restart (not just reloading the flight) is needed after rebuilding the panel package.
4. Build `C++/` (via CMake, or the provided `.sln`) and run `13-4-CommBusFromToolbar.exe`
   from a terminal *before* sending - it needs to already be connected and subscribed.
5. Start a flight, open the toolbar, and click the "CommBus Sender" icon.
6. Type a message, toggle the checkbox, click **Send**. The panel's status line confirms
   what was sent as JSON; the terminal running the C++ listener prints what it received.

If nothing arrives, use Developer Mode's **Debug > Debug Platform Dispatcher** to confirm
the event is actually being broadcast before assuming the C++ side is broken.

## Notes for anyone building a similar panel

A few things about MSFS's `InGamePanels`/toolbar-panel JS framework turned out to be
undocumented (the SDK's JavaScript docs cover VCockpit gauges almost exclusively) and had to
be worked out from FSDT GSX Pro's and PMDG's shipped, unminified panel code:

- **`RegisterCommBusListener()` needs `CommBus.js` loaded first**, and it is not a built-in
  global. The SDK docs' own suggested `Include.addScript("/JS/Services/CommBus.js")` call is
  asynchronous with no documented completion callback or reliable ordering guarantee relative
  to a panel's own script. `CommBusSender.js` polls for `RegisterCommBusListener` to actually
  exist rather than assuming any particular load order - this worked reliably where trying to
  force a specific script-tag load order did not.
- **`<ingame-ui content-fit="true">` doesn't reliably auto-size from content** in this
  version of the engine - relying on it left the panel collapsed to just its title bar.
  `CommBusSender.css` instead gives the frame an explicit fixed size and forces the
  framework-generated `.ingameUiContent`/`.ingameUiWrapper` wrapper divs to fill it (the same
  override GSX Pro's own shipped CSS applies to its panel).
