
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

if (NOT CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()


IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" ON)
else()
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" OFF)
ENDIF()

option(ASAN_SANITIZE_STACKS "Compile with fiber stacks sanitizing ON (If supported, implies ASAN)" ON)

option(USE_TSAN "Compile with Thread Sanitizer ON" OFF)
option(USE_USAN "Compile with UB Sanitizer ON (Default with Debug build)" OFF)


option(BUILD_TESTS    "Build Unit/Functional tests" OFF)
option(BUILD_SANDBOX  "Build selected programs in the project's sandbox" OFF)
#option(BUILD_STATIC   "Build statically linked binaries" OFF)
option(BUILD_EXAMPLES "Build project examples" OFF)


option(ICU_DEBUG "Debug FindICU" OFF)

set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

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

    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DBOOST_NO_AUTO_PTR -Wall -Wno-sign-compare -DJEMALLOC_NO_PRIVATE_NAMESPACE -D_GNU_SOURCE -D_REENTRANT -fPIC -DPIC" )
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DBOOST_STACKTRACE_USE_ADDR2LINE -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE  " )

    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors  -ftemplate-backtrace-limit=0") 
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -Wno-comment -Wno-unknown-warning-option -Wunused-variable")
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -Wno-unused-function -Wno-unused-but-set-variable")# 
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -Wno-unused-private-field -Wno-pessimizing-move")# 
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -Wno-missing-braces")
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -Wno-variables")
    
    set (MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} -pthread")
    
    if (USE_ASAN)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsanitize=address -DASAN_ENABLED")
        set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -fsanitize=address ") 
        
        if (ASAN_SANITIZE_STACKS)
            set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMMA1_SANITIZE_STACKS")
        endif() 
    endif()
    
    if (USE_USAN) 
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsanitize=undefined")
        set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -fsanitize=undefined")
    endif()
    
    if (USE_TSAN) 
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsanitize=thread")
        set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -fsanitize=thread")
    endif()
    
    set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -luuid -lstdc++ -lm") 
        
    if (MINGW)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -Wa,-mbig-obj ")

        set (MEMORIA_LIBS "${MEMORIA_LIBS}" rpcrt4 winpthread)
    else()
        set (MEMORIA_LIBS "${MEMORIA_LIBS}" dl )
    endif()
        
        
    if (BUILD_STATIC)
    	set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -static")
    	set (MEMORIA_LINK_FLAGS  "${MEMORIA_LINK_FLAGS} -static-libgcc -static-libstdc++ -static")
    endif()



        
else()
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} /W3 /std:c++14 -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -DJEMALLOC_NO_PRIVATE_NAMESPACE -D_REENTRANT -DDLLEXPORT -DNOMINMAX  /bigobj")
    set (MEMORIA_LIBS "${MEMORIA_LIBS}" rpcrt4 ) 
endif()


set (Boost_USE_DEBUG_LIBS	 ON)
set (Boost_USE_RELEASE_LIBS	 ON)
set (Boost_USE_DEBUG_RUNTIME	 ON)
set (Boost_USE_MULTITHREADED     ON)

#set (Boost_DETAILED_FAILURE_MSG  ON)
#set (Boost_DEBUG 		 ON)
#set (Boost_COMPILER -vc141)


if (MSVC)
    set (Boost_USE_STATIC_RUNTIME    OFF)
    set (Boost_USE_STATIC_LIBS       ON)
    find_package(ICU 60.2 COMPONENTS dt i18n uc REQUIRED)
else()
    set (Boost_USE_STATIC_RUNTIME    OFF)
    set (Boost_USE_STATIC_LIBS       OFF)
    find_package(Backtrace COMPONENTS backtrace)
    find_package(ICU 60.2 COMPONENTS i18n data uc REQUIRED)
endif()


find_package(Boost 1.66.0 COMPONENTS filesystem context program_options REQUIRED) 

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
set (MEMORIA_LIBS "${MEMORIA_LIBS}" ${Boost_LIBRARIES})


include_directories(${ICU_INCLUDE_DIRS})
link_directories(${ICU_LIBRARY_DIRS})
set (MEMORIA_LIBS "${MEMORIA_LIBS}" ${ICU_LIBRARIES})


if (Backtrace_FOUND)
    include_directories(${Backtrace_INCLUDE_DIRS})
    link_directories(${Backtrace_LIBRARY_DIRS})
endif()






set_property (GLOBAL PROPERTY GLOBAL_SOURCES)

add_subdirectory(src)
