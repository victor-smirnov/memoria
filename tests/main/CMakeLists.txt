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

file(GLOB TESTS_SOURCES *.cpp *.hpp)

add_executable(MainTests)
set_target_properties(MainTests PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(MainTests PROPERTIES OUTPUT_NAME tests)
target_link_libraries(MainTests TestsRunnerLib)

target_sources(
    MainTests
    PRIVATE ${TESTS_SOURCES}
)

install(TARGETS
  MainTests
  EXPORT MemoriaTargets FILE_SET PROGRAMS
)

