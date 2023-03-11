# Copyright 2023 Victor Smirnov
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

file(GLOB RUNNER_SOURCES *.cpp)
file(GLOB_RECURSE RUNNER_HEADERS include/*.hpp)

add_library(TestsRunnerLib STATIC)
set_target_properties(TestsRunnerLib PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(TestsRunnerLib PROPERTIES OUTPUT_NAME memoria_tests_runner_lib)

target_link_libraries(TestsRunnerLib Core AsioLib)
target_include_directories(TestsRunnerLib PUBLIC include)

target_sources(
    TestsRunnerLib
    PRIVATE ${RUNNER_SOURCES}
    PUBLIC  ${RUNNER_HEADERS}
)
