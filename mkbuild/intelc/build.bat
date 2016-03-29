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
CALL env.bat

SET start_time=%time%
ECHO Build started: %date%%start_time% > build_time.txt
SET start_secs=%start_time:~6,2%
SET start_mins=%start_time:~3,2%
SET start_hour=%start_time:~0,2%
SET /a start_time=(%start_hour%*60*60)+(%start_mins%*60)+(%start_secs%)

nmake 2>build_log.txt

SET end_time=%time%
ECHO Build finished: %date%%end_time% >> build_time.txt
SET end_secs=%end_time:~6,2%
SET end_mins=%end_time:~3,2%
SET end_hour=%end_time:~0,2%
IF %end_hour% LSS %start_hour% SET /a end_hour+=24
SET /a end_time=(%end_hour%*60*60)+(%end_mins%*60)+(%end_secs%)

SET /a duration=(%end_time%-%start_time%)
SET /a duration_hours=(%duration%)/3600
SET /a duration=(%duration%)%%3600
SET /a duration_mins=(%duration%)/60
SET /a duration_secs=(%duration%)%%60

IF %duration_secs% LSS 10 SET duration_secs=0%duration_secs%
IF %duration_mins% LSS 10 SET duration_mins=0%duration_mins%

SET duration=%duration_hours%:%duration_mins%:%duration_secs%

ECHO Duration: %duration%>> build_time.txt
ECHO Build duration: %duration%
