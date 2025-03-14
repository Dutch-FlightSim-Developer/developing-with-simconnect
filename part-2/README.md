# Part 2

# Message handling

## 2-1: [Polling for Messages in C](<./2-1 Polling for Messages in C/>)

This is an example of how to get messages back from Flight Simulator, using a simple (and inefficient) "polling" strategy. You can compare this to the infamous "Are we there yet?" scenario from the back seat of the car. The computer won't mind you doing this, but it is a waste of resources to continuously keep asking.

## 2-2: [Polling for Messages in C++](<./2-2 Polling for Messages in C++/>)

This does the same polling strategy, but now using our (cleaner) C++ API. To handle incoming messages, we use a `SimpleHandler` class defined in "`[simple_handler.hpp](../include/simconnect/simple_handler.hpp)`". This handler allows us to provide it with procedures or lambdas to do the actual handling, for each specific message type.

## 2-3: [Windows Messaging in C](<./2-3 Windows Messaging in C/>)

If you write an old-school Win32 desktop application in C, you can get your messages through the Windows Message queue of your (main) window. This is more efficient, because you don't have to continuously check for new messages.

## 2-4: [Windows Messaging in C++](<./2-4 Windows Messaging in C++/>)

The same strategy will also work in C++, so to keep to that language, this example uses an MFC (Microsoft Foundation Classes) version of the same dialog window, coupled with our C++ API. To support the changed call to `SimConnect_Open`, we have a specialized version of the `Connection` class, named `WindowsMessagingConnection`, which will create the Event if we don't want to. It is defined in "`[windows_messaging_connection.hpp](../include/simconnect/windows_messaging_connection.hpp)`". The handler is the same `SimpleHandler` of the previous example.

## 2-5: [Windows Event in C](<./2-5 Windows Event in C/>)

Microsoft FlighhtSimulator allows you to use a Windows Event to signal you when there are messages. The "Event" is an unfortunate misnomer, as it actually is what is commonly referred to as a "Condition Variable." As long as we accept the Win32 API, this is a lot better than polling, because our application will actually wait until it gets signalled.

The source file for this demo is again from the MSFS 2024 SDK Samples, named "WindowsEvent".

## 2-6: [Windows Event in C++](<./2-6 Windows Event in C++/>)

Naturally we can hide the Windows Event inside our C++ API. This uses another specialized version of the `Connection` class, named `WindowsEventConnection`, which will create the Event if we don't want to. It is defined in "`[windows_event_connection.hpp](../include/simconnect/windows_event_connection.hpp)`", together with a specialized `WindowsEventHandler` defined in "`[windows_event_handler.hpp](../include/simconnect/windows_event_handler.hpp)`".

## 2-7: [Request the System State in C](<./2-7 RequestSystemState in C/>)

This program will connect to the simulator, and then proceed to request all 5 different states. Because this is done by name, it also requests a 6th one with an unknown System State, to show you what the response is.

## 2-8: [Request the System State in C++](<./2-8 RequestSystemState in C++/>)

This example does the same, but now with our C++ API.

## 2-9: [Exception Messages and SendID in C](<./2-9 Exception Messages and SendID in C/>)

This is basically the same as the "RequestSystemState" example, but now with a handler for Exception messages.

## 2-10: [Exception Messages and SendID in C++](<./2-10 Exception Messages and SendID in C++/>)

This is basically the same as the "RequestSystemState" example, but now with a handler for Exception messages.

As a small "proof of concept", the Solution builds a 64-bit version as usual, but a 32-bit version based on the FSX SimConnect SDK. That project will fail if you do not have it, or if you don't have a `FSX_SDK` environment variable pointing at it. If you happen to have "FSX for Steam", which will run on Windows 11, make sure you have the environment variable pointing at "_<drive>_`:\SteamLibrary\steamapps\common\FSX\SDK\Core Utilities Kit`", with "_<drive>_" being the drive where your Steam Library is. You should also run the installer for the SimConnect DLL, which can be found in "`${FSX_SDK}\SimConnect SDK\lib`".

## 2-11: [SubscribeToSystemEvent in C](<./2-11 SubscribeToSystemEvent in C/>)

This is a project with the default "SubScribeToSystemEvent" Sample.

## 2-12: [SubscribeToSystemEvent in C++](<./2-12 SubscribeToSystemEvent in C++/>)

And here is a C++ version.
