@ECHO OFF

SET BASE_DIR=%~dp0
SET BASE_DIR=%BASE_DIR:~0,-`%
SET BUILD_DIR=%BASE_DIR%\..\..\..\memoria-build\mingw

MKDIR %BUILD_DIR%

COPY /-Y %BASE_DIR%\*.bat %BUILD_DIR%
DEL %BUILD_DIR%\setup.bat
