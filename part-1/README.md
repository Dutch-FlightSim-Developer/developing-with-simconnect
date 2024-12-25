# Part 1 - Setting up the connection

These examples assume you have Microsoft FlightSimulator 2024 and its SDK installed, as well the Visual Studio 2022, as described in the SDK documentation. You should also install the "Samples" package.

As a result of the installation, you will have an environment variable named `MSFS2024_SDK` defined on your PC, with as its value the path to the SDK.

## 1-1 : The default "OpenClose" example

You will find here onlye the project files for this sample, named "DefaultOpenClose":

* `DefaultOpenClose.sln` is the Visual Studio "Solution" file. It just contains the reference to the actual project.
* `DefaultOpenClose.vcxproj` is the actual project, which contains a reference to the source file in the SDK. It uses the environment variable to locate the file, even though it will build the project here.
* `DefaultOpenClose.vcxproj.filters` defines the groups you see in the "Solution" view.

## 1-2 : The C++ version of the "OpenClose" example

This version is an example of how you would write the "OpenClose" demo in C++.

## 1-3 : Polling for Messages in C

This is an example of how to get messages back from Flight Simulator, using a simple (and inefficient) "polling" strategy.

## 1-4 : Polling for Messages in C++

This does the same polling strategy, but now using our (cleaner) C++ API.

## 1-5 : Windows Messaging in C

If you write an old-school Win32 desktop application in C, you can get your messages through the Windows Message queue of your (main) window. This is more efficient, because you don't have to continuously check for new messages.

## 1-6 : Windows Messaging in C++

The same strategy will also work in C++, so to keep to that language, this example uses an MFC (Microsoft Foundation Classes) version of the same dialog window, coupled with our C++ API.

## 1-7 : Windows Event in C

Microsoft FlighhtSimulator allows you to use a Windows Event to signal you when there are messages. The "Event" is an unfortunate misnomer, as it actually is what is commonly referred to as a "Condition Variable." As long as we accept the Win32 API, this is a lot better than polling, because our application will actually wait until it gets signalled.

## 1-8 : Windows Event in C++

Naturally we can hide the Windows Event inside our C++ API.