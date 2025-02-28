# Part 1 - Setting up the connection and handling incoming messages.

These examples assume you have Microsoft FlightSimulator 2024 and its SDK installed, as well the Visual Studio 2022, as described in the SDK documentation. You should also install the "Samples" package.

As a result of the installation, you will have an environment variable named `MSFS2024_SDK` defined on your PC, with as its value the path to the SDK.

## 1-1: [The default "OpenClose" example](<./1-1 OpenClose in C/>)

This is an example of simply opening the connection to the simulator and closing it. You will find here only the project files for this sample, as the source is in the SDK Samples, named "OpenClose".

## 1-2: [The C++ version of the "OpenClose" example](<./1-2 OpenClose in C++/>)

This version of "OpenClose" is an example of how you would write it in C++, using my header files. We use a `SimpleConnection` class to set up the connection, which provides us with simplified "`open`" and "`close`" methods, and hides the SimConnect `HANDLE`. It is defined in the "`[simple_connection.hpp](../include/simconnect/simple_connection.hpp)`" header file.
