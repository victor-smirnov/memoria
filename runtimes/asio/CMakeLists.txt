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

file(GLOB LIB_SOURCES *.cpp hrpc/*.cpp)
file(GLOB_RECURSE LIB_HEADERS include/*.hpp)

set_target_properties(AsioLib PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(AsioLib PROPERTIES OUTPUT_NAME memoria_asio)

target_link_libraries(AsioLib PUBLIC RuntimeApi Boost::program_options Boost::context Boost::fiber)
target_include_directories(AsioLib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/asio/include>
)
target_compile_definitions(AsioLib PUBLIC -DMEMORIA_ASIO_RUNTIME)

target_sources(
  AsioLib
  PRIVATE ${LIB_HEADERS}
  PRIVATE ${LIB_SOURCES}
)

install(TARGETS AsioLib EXPORT MemoriaTargets)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/asio
)
