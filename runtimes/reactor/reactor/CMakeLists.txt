# Copyright 2017-2025 Victor Smirnov
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


if (BUILD_MACOSX)
    file(GLOB LIB_SOURCES *.cpp macosx/*.cpp posix/*.cpp)
elseif (BUILD_MSVC)
    file(GLOB LIB_SOURCES *.cpp msvc/*.cpp)
else()
    file(GLOB LIB_SOURCES *.cpp linux/*.cpp posix/*.cpp)
endif()

file(GLOB HRPC_SOURCES hrpc/*.cpp)

target_sources(ReactorLib PRIVATE ${LIB_SOURCES} ${HRPC_SOURCES})
