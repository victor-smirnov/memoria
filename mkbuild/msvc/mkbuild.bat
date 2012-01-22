@ECHO OFF

MKDIR lib
SET PWD=%cd%

CALL env.bat

SET BASE_DIR=%~dp0
SET BASE_DIR=%BASE_DIR:~0,-1%

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release %BASE_DIR%\..\..\memoria