#!/bin/bash

## Copyright 2012 Memoria team
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.

BASE=../../../memoria

INCS="-I $BASE/include -I $BASE/src/core/include -I $BASE/src/tools/include"

LIBS="-l $BASE/src/core/src/reflection.cpp\
     -l $BASE/src/core/src/stream.cpp\
     -l $BASE/src/core/src/exceptions.cpp\
     -l $BASE/src/core/src/strings.cpp\
     -l $BASE/src/core/src/models.cpp
     -l $BASE/src/core/src/md5.cpp\
     -l $BASE/src/core/src/file.cpp\
     -l $BASE/src/core/src/metadata_tools.cpp\
     -l $BASE/src/core/platforms/posix/strings.cpp\
     -l $BASE/src/core/platforms/posix/platform.cpp\
     -l $BASE/src/core/platforms/posix/logs.cpp\
     -l $BASE/src/core/platforms/posix/file.cpp\
     -l $BASE/src/allocators/inmem/config.cpp\
     -l $BASE/src/tools/lib/benchmarks.cpp\
     -l $BASE/src/tools/lib/cmdline.cpp\
     -l $BASE/src/tools/lib/configuration.cpp\
     -l $BASE/src/tools/lib/params.cpp\
     -l $BASE/src/tools/lib/task.cpp\
     -l $BASE/src/tools/lib/tests.cpp\
     -l $BASE/src/tools/lib/tools.cpp\
     -l $BASE/src/tools/tests/tests.cpp"
     
/opt/llvm/bin/cling ${LIBS} -cc1 ${INCS} -std=c++11 -DMEMORIA_SRC $BASE/src/tools/tests/cling_tests.cpp


#clingtest.cpp
