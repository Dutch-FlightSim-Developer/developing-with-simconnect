# CommBus Message Viewer (EFB app)

A minimal Electronic Flight Bag app that listens for the CommBus event
`DutchFlightSim.Tutorial.Hello` and displays the last message received. It's meant to be
fed by the `../C++/` sender (or the `13-1` C caller, which broadcasts the same event name).

This folder mirrors the structure of the MSFS 2024 SDK's own `EFB_Template_Sample`
(`<SDK>\Samples\DevmodeProjects\EFB\`). Unlike every other example in this repository,
it is **not** built through CMake - it needs Node/npm and MSFS Developer Mode.

## One-time setup

This app depends on two folders that ship with the SDK sample and are too large/binary
to duplicate into this repo. Copy them from your SDK install so they sit next to
`PackageSources\CommBusMessageViewer\` as siblings:

```
<SDK>\Samples\DevmodeProjects\EFB\PackageSources\efb_api    ->  PackageSources\efb_api
<SDK>\Samples\DevmodeProjects\EFB\PackageSources\vendor     ->  PackageSources\vendor
```

After copying, this folder should look like:

```
EFB/
  CommBusToEFBProject.xml
  PackageDefinitions/
    dutchflightsim-efb-commbusmessageviewer.xml
  PackageSources/
    efb_api/       <- copied from the SDK sample
    vendor/        <- copied from the SDK sample
    CommBusMessageViewer/
      src/
      package.json
      build.js
      .env
      tsconfig.json
```

Then, from `PackageSources\CommBusMessageViewer\`:

```
npm install
npm run build
```

This produces a `dist/` folder, which the package definition's `Copy` asset group picks
up.

## Building the package

1. Enable Developer Mode in MSFS 2024 (see the SDK docs if you haven't already).
2. **File > Open project...** and select `CommBusToEFBProject.xml`.
3. In the Project Editor, click **Build All In Project**. Do this *before* starting a
   flight - packages aren't hot-reloaded into an already-loaded EFB.

## Testing

1. Start a flight in an aircraft with an EFB (the DA62 is a good default - it has the
   full modeled EFB rather than the 2D legacy panel).
2. Open **CommBus Message Viewer** from the EFB's app list.
3. From a terminal, run the `13-3` C++ sender (built via this repo's normal CMake
   pipeline) with a message:

   ```
   13-3-CommBusToEFB-Sender.exe Hello from the terminal!
   ```

4. The message should appear in the app within a second or two.

If nothing shows up, open Developer Mode's **Debug > Debug Platform Dispatcher** to watch
CommBus traffic and confirm the event is actually being broadcast and reaching JS
listeners - see the "Known Issues" and troubleshooting notes on the SDK's Communication
API docs page if it looks stuck.

## Status

Confirmed working end-to-end in Developer Mode: running the C++ sender updates the
message shown live in the EFB app. Three bugs turned up along the way and are fixed in the
current source - noted here in case similar issues show up in future EFB examples:

- The `App` subclass must override `install()` and call
  `Efb.loadCss(`${BASE_URL}/CommBusMessageViewer.css`)`, matching the SDK's own
  `TemplateApp.tsx`. Without it, the compiled CSS is built into `dist/` but never applied to
  the page, so the view renders with unstyled (black) text against the EFB's dark
  background - visually indistinguishable from not rendering at all.
- `RegisterCommBusListener()` (from including `CommBus.js`, per the SDK's JS Communication
  API docs and the `CommBusAircraft` sample gauge) throws
  `Can't find variable: RegisterCommBusListener` when run inside an EFB page - confirmed via
  the Coherent GT Debugger's console on the live "Electronic Flight Bag" webview. It seems
  to only initialize correctly in a VCockpit gauge context. Registering directly against the
  underlying Coherent view listener it wraps instead works in the EFB:
  `RegisterViewListener("JS_LISTENER_COMM_BUS")`, per
  [this forum thread](https://devsupport.flightsimulator.com/t/is-commbus-available-in-efb/15285/3).
  `RegisterViewListener`/`ViewListener` are already typed by `@microsoft/msfs-types`, so no
  ambient declarations or `CommBus.js` include are needed.
- `build.js`'s `postcss-prefix-selector` plugin prefixes the compiled CSS with
  `.efb-view.${__dirname folder name}` (here, `.efb-view.CommBusMessageViewer`), copied
  verbatim from the SDK's `TemplateApp` sample. The EFB host wraps each mounted app's root in
  a `<div class="efb-view <AppClassName>">`, where `<AppClassName>` comes from the `App`
  subclass's own class name - **not** the folder name. `TemplateApp`'s sample happens to name
  its `App` subclass `TemplateApp`, identical to its folder, which hides this coupling. This
  app originally named its `App` subclass `CommBusMessageViewerApp`, which produced a
  wrapper class of `efb-view CommBusMessageViewerApp` that the prefixed CSS
  (`efb-view CommBusMessageViewer`) never matched - confirmed via the DOM inspector, which
  showed the correctly-rendered, correctly-updating (`.last-message` really did contain the
  broadcast text) but completely unstyled markup. Renamed the `App` subclass to
  `CommBusMessageViewer` to match the folder name and restore the match.
