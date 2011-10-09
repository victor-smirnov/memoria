@echo off
call env.bat

powershell -Command "&{Get-Date}" >build_time.txt
mingw32-make 2>build_log.txt
powershell -Command "&{Get-Date}" >>build_time.txt