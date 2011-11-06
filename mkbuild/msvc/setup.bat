@ECHO OFF

SET BUILD=..\..\..\memoria-build\msvc

MKDIR %BUILD%

COPY /-Y *.bat %BUILD%
DEL %BUILD%\setup.bat