# Templates for Visual Studio 2022 projects

The templates in these directories can be installed by putting the ZIPped contents of the template directory (without the directory itself!) into your Documents folder, under "`Visual Studio 2022\Templates\Project Templates\Visual C++`".

##  MSFS 2020 Console - static SimConnect libs

This will create a project with the following settings:

* Using C++20 and C17 language standards
* Warning level 4, Warnings are treated as errors
* Load the "static libs" property sheet from the MSFS 2020 SDK.

** Note that this assumes the `MSFS_SDK` environment variable to be set.

The project contains an example `main.cpp` file that connects and then disconnects.

##  MSFS 2024 Console - static SimConnect libs

This will create a project with the following settings:

* Using C++20 and C17 language standards
* Warning level 4, Warnings are treated as errors
* Load the "static libs" property sheet from the MSFS 2024 SDK.

** Note that this assumes the `MSFS2024_SDK` environment variable to be set.

The project contains an example `main.cpp` file that connects and then disconnects.
