
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


add_library(ContainersApi STATIC)

set_target_properties(ContainersApi PROPERTIES CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})
set_target_properties(ContainersApi PROPERTIES OUTPUT_NAME memoria_containers_api)

target_link_libraries(ContainersApi PUBLIC Core)
target_include_directories(ContainersApi PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/containers-api/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

file(GLOB CTR_API_SOURCES *.cpp)
file(GLOB_RECURSE CTR_API_HEADERS include/*.*)

target_sources(ContainersApi
    PRIVATE FILE_SET HEADERS FILES ${CTR_API_HEADERS}
    PRIVATE ${CTR_API_SOURCES}
)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/containers-api
)

install(TARGETS ContainersApi EXPORT MemoriaTargets)
