
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

set_target_properties(Containers PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(Containers PROPERTIES OUTPUT_NAME memoria_containers)

target_link_libraries(Containers PUBLIC ContainersApi)

target_include_directories(Containers PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/containers/include>
)

file(GLOB CTR_SOURCES *.cpp)
file(GLOB_RECURSE PUBLIC_CTR_HEADERS include/*.*)

target_sources(Containers
    PRIVATE FILE_SET HEADERS FILES ${PUBLIC_CTR_HEADERS}
    PRIVATE ${GENERATED_MAIN_SOURCES} ${CTR_SOURCES}
)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/containers
)

install(TARGETS Containers EXPORT MemoriaTargets)
