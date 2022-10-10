
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

file(GLOB SRCS1 src/*.cpp)
file(GLOB SRCS2 src/datatypes/*.cpp)


file(GLOB SRCS3 src/hermes/path/ast/*.cpp src/hermes/path/*.cpp src/hermes/*.cpp)
file(GLOB SRCS4 src/hermes/path/interpreter/*.cpp)
file(GLOB SRCS5 src/hermes/path/parser/*.cpp)

set_property(GLOBAL APPEND PROPERTY GLOBAL_CORE_SOURCES ${SRCS1} ${SRCS2} ${SRCS3} ${SRCS4} ${SRCS5})
set_property(GLOBAL APPEND PROPERTY GLOBAL_CORE_SOURCES_REACTOR ${SRCS1} ${SRCS2} ${SRCS3} ${SRCS4} ${SRCS5})

add_subdirectory(platforms)
