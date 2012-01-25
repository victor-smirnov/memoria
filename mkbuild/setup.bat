@ECHO OFF

REM default is mingw
SET SUBDIR=empty
IF "%1"=="" (
    SET SUBDIR=mingw
) ELSE (
    IF "%2"=="" (
        IF "%1"=="--mingw" ( SET SUBDIR=mingw )
        IF "%1"=="--msvc" ( SET SUBDIR=msvc )
        IF "%1"=="--intelc" ( SET SUBDIR=intelc )
    )
)

IF "%SUBDIR%"=="empty" ( GOTO usage )

SET BASE_DIR=%~dp0
SET BASE_DIR=%BASE_DIR:~0,-1%
SET BUILD_DIR=%BASE_DIR%\..\..\memoria-build\%SUBDIR%

MKDIR %BUILD_DIR%

COPY /-Y %BASE_DIR%\%SUBDIR%\*.bat %BUILD_DIR%
DEL %BUILD_DIR%\setup.bat

EXIT /B 0

:usage
SET SCRIPT_NAME=setup.sh
ECHO USAGE: %SCRIPT_NAME% [--mingw^|--msvc^|--intelc]
ECHO Default is --mingw
EXIT /B 1