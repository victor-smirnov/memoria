
# Copyright 2016-2025 Victor Smirnov
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

find_package(yaml-cpp REQUIRED)

file(GLOB TOOLS_LIB_SRC *.cpp)

add_library(Tests2 STATIC )
set_target_properties(Tests2 PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(Tests2 PROPERTIES OUTPUT_NAME memoria_tests2)

target_link_libraries(Tests2 Core AsioLib yaml-cpp)
target_include_directories(Tests2 PUBLIC ../include)

file(GLOB_RECURSE _HDRS "../include/*.*")
target_sources(
    Tests2
    PRIVATE ${TOOLS_LIB_SRC}
    PUBLIC ${_HDRS}
)



