@ECHO OFF
MKDIR lib
SET PWD=%cd%

CALL env.bat

REM set PATH=d:\cmake28\bin

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="C:/Program Files (x86)/Intel/Composer XE 2011 SP1/bin/ia32/icl.exe" -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/Intel/Composer XE 2011 SP1/bin/ia32/icl.exe"  -DMEMORIA_TESTS="tree_map" -DBUILD_TOOLS=false ..\..\memoria