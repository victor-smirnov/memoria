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

# MCP server build script

find_package(Clang CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)


add_executable(mcp mcp_main.cpp mcp_server.cpp mcp_stdio_transport.cpp mcp_http_transport.cpp)

target_include_directories(mcp PRIVATE include)
target_include_directories(mcp PRIVATE ${CLANG_INCLUDE_DIRS})
target_include_directories(mcp PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(mcp PRIVATE Core)
target_link_libraries(mcp PRIVATE Boost::program_options Boost::system)
target_link_libraries(mcp PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(mcp PRIVATE unofficial::sqlite3::sqlite3)
target_link_libraries(mcp PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)
target_link_libraries(mcp PRIVATE Crow::Crow asio::asio)

install(TARGETS
  mcp
    EXPORT MemoriaTargets
    RUNTIME DESTINATION ${MEMORIA_TOOLS_INSTALL_DIR}
)
