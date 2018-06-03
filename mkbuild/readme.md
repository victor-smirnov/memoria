For MSVC/Clang builds:

1. Copy v141_clang6 to appropriate toolset folder of your MSVC installation if you plan to build Memoria with Clang 5.0+.

2. Copy mkvc-build.bat and/or mkvc-clang6-build.bat to the same level as memoria folder. Running these scripts will create MSVC projects for Memoria. 

Boost 1.66+ and ICU must be installed and available for CMake (present in Path). Qt is optional but used for Datascope.