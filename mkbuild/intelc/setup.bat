@ECHO OFF

SET BUILD=..\..\..\memoria-build\intelc

MKDIR %BUILD%

COPY /-Y *.bat %BUILD%
DEL %BUILD%\setup.bat