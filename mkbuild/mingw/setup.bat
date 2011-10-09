set BUILD=..\..\..\memoria-build\mingw

mkdir %BUILD%

copy /-Y *.bat %BUILD%
del %BUILD%\setup.bat