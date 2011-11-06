@ECHO OFF

SET BUILD=..\..\..\memoria-build\mingw

MKDIR %BUILD%

COPY /-Y *.bat %BUILD%
DEL %BUILD%\setup.bat