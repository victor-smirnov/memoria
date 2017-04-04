
# Copyright 2016 Victor Smirnov
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# available variables to be used via -D switch in command line:
# 
# MEMORIA_COMPILE_FLAGS            Additional flags for compiler
# MEMORIA_LINK_FLAGS               Additional flags for linker

cmake_minimum_required (VERSION 3.6)
project (Memoria)

set(VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 14)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ASAN_DEFAULT ON)
    set(USAN_DEFAULT ON)
    set(TSAN_DEFAULT OFF)
else()
    set(ASAN_DEFAULT OFF)
    set(USAN_DEFAULT OFF)
    set(TSAN_DEFAULT OFF)
ENDIF()


option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" ${ASAN_DEFAULT})
option(USE_TSAN "Compile with Thread Sanitizer ON" ${TSAN_DEFAULT})
option(USE_USAN "Compile with UB Sanitizer ON (Default with Debug build)" ${USAN_DEFAULT})

option(BUILD_TESTS    "Build Unit/Functional tests" OFF)
option(BUILD_SANDBOX  "Build selected programs in the project's sandbox" OFF)
option(BUILD_TOOLS    "Build project's tools" ON)

option(BUILD_STATIC   "Build statically linked binaries" OFF)

set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set (BUILD_UNIX   FALSE)
set (BUILD_MSVC   FALSE)

if (UNIX)
    set (BUILD_UNIX TRUE)
elseif(MSVC)
    set (BUILD_MSVC TRUE)
elseif(MINGW)
else()
    message(FATAL_ERROR "This platform is not supported")
endif()





if (BUILD_UNIX OR MINGW)

    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
    SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -Wall -Wno-sign-compare")

    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors  -ftemplate-backtrace-limit=0 ${PROFILE}") #-fsanitize=undefined -fno-sanitize-recover
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -Wcomment -Wunused-variable -Wno-unused-but-set-variable -Wno-unused-function -Wno-unneeded-internal-declaration")
    
    set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} ${PROFILE} -luuid -lstdc++") #-lubsan
        
    if (MINGW)
        set (MEMORIA_LIBS "${MEMORIA_LIBS}" rpcrt4 winpthread)
    else()
        set (MEMORIA_LIBS "${MEMORIA_LIBS}")
    endif()
        
        
    if (BUILD_STATIC)
    	set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -static")
    	set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -static-libgcc -static-libstdc++ -static")
    endif()
    
    if (BUILD_ASAN)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsanitize=address")
        set (MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} -fsanitize=address")
    endif()
    
else()
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} /W3 /std:c++14 -DWIN32_LEAN_AND_MEAN")
    set (MEMORIA_LIBS "${MEMORIA_LIBS}" rpcrt4 )
endif()


set(Boost_USE_STATIC_LIBS        ON) 
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     ON)

find_package(Boost
  1.60.0
)   

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

set_property (GLOBAL PROPERTY GLOBAL_SOURCES)

add_subdirectory(src)

include(CPack)
