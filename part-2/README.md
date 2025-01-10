# Part 2 - System State and Events

In this part we look at our first interactions with the simulator:

* The System State is represented by a number of different values we can request, and it tells us (globally) what the user is doing in the simulator.
* Events are messages that inform us about things that have happened. This can tell us what the user did, or about things that happen in the simulator. Add-ons can also send events.

## System State

You can ask the Simulator to tell you what state it is in. The "states" you can enquire about are:

* **AircraftLoaded**

  This will tell you the currently selected aircraft's flight dynamics, by providing you with the path to its file.

* **DialogMode**

  This will tell you if the user is in a dialog or not.

* **FlightLoaded**

  This will give you the path to the currently loaded flight. For a "Free flight", a file will be made with the information the user has entered.

* **FlightPlan**

  This will give you the path to the currently selected flightplan. For a "Free flight", a file will be made with the information the user has entered.

  * **Sim**

    This will tell you if a simulation is currently running.

## 2-1: [Request the System State in C](<./2-1 RequestSystemState in C/>)

This program will connect to the simulator, and then proceed to request all 5 different states. Because this is done by name, it also requests a 6th one with an unknown System State, to show you what the response is.

## 2-2: [Request the System State in C++](<./2-2 RequestSystemState in C++/>)

This example does the same, but now with our C++ API.

## 2-3: [Exception Messages and SendID in C](<./2-3 Exception Messages and SendID in C/>)

This is basically the same as the "RequestSystemState" example, but now with a handler for Exception messages.

## 2-4: [Exception Messages and SendID in C++](<./2-4 Exception Messages and SendID in C++/>)

This is basically the same as the "RequestSystemState" example, but now with a handler for Exception messages.

As a small "proof of concept", the Solution builds a 64-bit version as usual, but a 32-bit version based on the FSX SimConnect SDK. That project will fail if you do not have it, or if you don't have a `FSX_SDK` environment variable pointing at it. If you happen to have "FSX for Steam", which will run on Windows 11, make sure you have the environment variable pointing at "_<drive>_`:\SteamLibrary\steamapps\common\FSX\SDK\Core Utilities Kit`", with "_<drive>_" being the drive where your Steam Library is. You should also run the installer for the SimConnect DLL, which can be found in "`${FSX_SDK}\SimConnect SDK\lib`".

## 2-5: [SubscribeToSystemEvent in C](<./2-5 SubscribeToSystemEvent in C/>)

This is a project with the default "SubScribeToSystemEvent" Sample.

## 2-6: [SubscribeToSystemEvent in C++](<./2-6 SubscribeToSystemEvent in C++/>)

And here is a C++ version.
