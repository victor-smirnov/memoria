REM Copyright 2012 Memoria team
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM     http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.

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