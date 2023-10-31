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

file(GLOB_RECURSE LIB_HEADERS include/*.hpp)

set_target_properties(ReactorLib PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(ReactorLib PROPERTIES OUTPUT_NAME memoria_reactor)

target_link_libraries(ReactorLib PUBLIC RuntimeApi Boost::program_options Boost::context Boost::fiber)
target_include_directories(ReactorLib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/runtimes/reactor/include>
)
target_compile_definitions(ReactorLib PUBLIC -DMEMORIA_REACTOR_RUNTIME)


target_sources(
  ReactorLib
  PRIVATE FILE_SET HEADERS FILES ${LIB_HEADERS}
)

add_subdirectory(reactor)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/reactor
)

install(TARGETS ReactorLib EXPORT MemoriaTargets)
