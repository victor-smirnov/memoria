@echo off

cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_MODULES=false -DBUILD_GENERIC_CONTAINER=false -DBUILD_POSIX=false ../../memoria