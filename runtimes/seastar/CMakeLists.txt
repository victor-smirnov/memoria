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

message("===================================================== ${CMAKE_MODULE_PATH} ===================")

add_library (ucontext::ucontext INTERFACE IMPORTED)
add_library (SystemTap::SDT INTERFACE IMPORTED)

find_package(Seastar REQUIRED)

file(GLOB_RECURSE LIB_SOURCES *.cpp)
file(GLOB LIB_HEADERS *.hpp)

file(GLOB_RECURSE PUBLIC_LIB_HEADERS include/*.hpp)

set_target_properties(SeastarLib PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(SeastarLib PROPERTIES OUTPUT_NAME memoria_seastar)

target_link_libraries(SeastarLib PUBLIC RuntimeApi Seastar::seastar Boost::program_options)

target_include_directories(SeastarLib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/seastar/include>
)

target_compile_definitions(SeastarLib PUBLIC -DSEASTAR_SCHEDULING_GROUPS_COUNT=16 -DSEASTAR_API_LEVEL=7)
target_compile_definitions(SeastarLib PUBLIC -DMEMORIA_SEASTAR_RUNTIME)

target_sources(
  SeastarLib
  PRIVATE ${LIB_SOURCES} ${LIB_HEADERS}
  PRIVATE ${PUBLIC_LIB_HEADERS}
)

install(TARGETS
  SeastarLib EXPORT MemoriaTargets
)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/seastar
)
