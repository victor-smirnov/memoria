@ECHO OFF

MKDIR lib
SET PWD=%cd%

CALL env.bat

REM SET PATH=d:\cmake28\bin

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..\..\memoria