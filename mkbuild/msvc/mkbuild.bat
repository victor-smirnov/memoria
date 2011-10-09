@echo off
mkdir lib
set PWD=%cd%

call env.bat

rem set PATH=d:\cmake28\bin

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..\..\memoria