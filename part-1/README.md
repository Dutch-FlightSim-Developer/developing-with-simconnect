# Part 1 - Setting up the connection and handling incoming messages.

These examples assume you have Microsoft FlightSimulator 2024 and its SDK installed, as well the Visual Studio 2022, as described in the SDK documentation. You should also install the "Samples" package.

As a result of the installation, you will have an environment variable named `MSFS2024_SDK` defined on your PC, with as its value the path to the SDK.

## 1-1: [The default "OpenClose" example](<./1-1 OpenClose in C/>)

This is an example of simply opening the connection to the simulator and closing it. You will find here only the project files for this sample, as the source is in the SDK Samples, named "OpenClose".

## 1-2: [The C++ version of the "OpenClose" example](<./1-2 OpenClose in C++/>)

This version of "OpenClose" is an example of how you would write it in C++, using my header files. We use a `SimpleConnection` class to set up the connection, which provides us with simplified "`open`" and "`close`" methods, and hides the SimConnect `HANDLE`. It is defined in the "`[simple_connection.hpp](../include/simconnect/simple_connection.hpp)`" header file.

## 1-3: [Polling for Messages in C](<./1-3 Polling for Messages in C/>)

This is an example of how to get messages back from Flight Simulator, using a simple (and inefficient) "polling" strategy. You can compare this to the infamous "Are we there yet?" scenario from the back seat of the car. The computer won't mind you doing this, but it is a waste of resources to continuously keep asking.

## 1-4: [Polling for Messages in C++](<./1-4 Polling for Messages in C++/>)

This does the same polling strategy, but now using our (cleaner) C++ API. To handle incoming messages, we use a `SimpleHandler` class defined in "`[simple_handler.hpp](../include/simconnect/simple_handler.hpp)`". This handler allows us to provide it with procedures or lambdas to do the actual handling, for each specific message type.

## 1-5: [Windows Messaging in C](<./1-5 Windows Messaging in C/>)

If you write an old-school Win32 desktop application in C, you can get your messages through the Windows Message queue of your (main) window. This is more efficient, because you don't have to continuously check for new messages.

## 1-6: [Windows Messaging in C++](<./1-6 Windows Messaging in C++/>)

The same strategy will also work in C++, so to keep to that language, this example uses an MFC (Microsoft Foundation Classes) version of the same dialog window, coupled with our C++ API. To support the changed call to `SimConnect_Open`, we have a specialized version of the `Connection` class, named `WindowsMessagingConnection`, which will create the Event if we don't want to. It is defined in "`[windows_messaging_connection.hpp](../include/simconnect/windows_messaging_connection.hpp)`". The handler is the same `SimpleHandler` of the previous example.

## 1-7: [Windows Event in C](<./1-7 Windows Event in C/>)

Microsoft FlighhtSimulator allows you to use a Windows Event to signal you when there are messages. The "Event" is an unfortunate misnomer, as it actually is what is commonly referred to as a "Condition Variable." As long as we accept the Win32 API, this is a lot better than polling, because our application will actually wait until it gets signalled.

The source file for this demo is again from the MSFS 2024 SDK Samples, named "WindowsEvent".

## 1-8: [Windows Event in C++](<./1-8 Windows Event in C++/>)

Naturally we can hide the Windows Event inside our C++ API. This uses another specialized version of the `Connection` class, named `WindowsEventConnection`, which will create the Event if we don't want to. It is defined in "`[windows_event_connection.hpp](../include/simconnect/windows_event_connection.hpp)`", together with a specialized `WindowsEventHandler` defined in "`[windows_event_handler.hpp](../include/simconnect/windows_event_handler.hpp)`".