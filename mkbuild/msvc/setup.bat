set BUILD=..\..\..\memoria-build\msvc

mkdir %BUILD%

copy /-Y *.bat %BUILD%
del %BUILD%\setup.bat