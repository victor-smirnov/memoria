# Copyright 2025 Victor Smirnov
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

find_package(Clang CONFIG REQUIRED)

add_executable(mcp)

target_include_directories(mcp PRIVATE include)
target_include_directories(mcp PRIVATE ${CLANG_INCLUDE_DIRS})
target_include_directories(mcp PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(mcp PRIVATE Core)
target_link_libraries(mcp PRIVATE Boost::program_options Boost::filesystem)
target_link_libraries(mcp PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)

file(GLOB MCP_SOURCES *.cpp *.hpp)
file(GLOB_RECURSE MCP_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "include/*.*" )

target_sources(
  mcp
  PRIVATE ${MCP_SOURCES}
  PRIVATE ${MCP_HEADERS}
)

install(TARGETS
  mcp
    EXPORT MemoriaTargets
    RUNTIME DESTINATION ${MEMORIA_TOOLS_INSTALL_DIR}
)
