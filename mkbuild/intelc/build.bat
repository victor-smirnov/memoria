@call env.bat

powershell -Command "&{Get-Date}" >build_time.txt

nmake >build_log.txt

powershell -Command "&{Get-Date}" >>build_time.txt