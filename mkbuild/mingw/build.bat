@echo off
call env.bat

echo %time% > build_time.txt

mingw32-make 2> build_log.txt

echo %time% >> build_time.txt