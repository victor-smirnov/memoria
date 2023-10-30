
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

file(GLOB INIT_SOURCES *.cpp)
file(GLOB INIT_HEADERS include/*.hpp)

add_library(AppInit STATIC)

target_include_directories(AppInit PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/app_init/include>
)

target_sources(AppInit
  PRIVATE ${GLOBAL_INIT_SRCS} ${INIT_SOURCES}
  PRIVATE FILE_SET HEADERS FILES ${INIT_HEADERS}
)

target_link_libraries(AppInit Core Containers Stores)
set_target_properties(AppInit PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(AppInit PROPERTIES OUTPUT_NAME memoria_app_init)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/app_init
)

install(TARGETS AppInit EXPORT MemoriaTargets)
