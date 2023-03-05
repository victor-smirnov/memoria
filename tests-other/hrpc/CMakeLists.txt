
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

file(GLOB LIB_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} hrpc_tests_common.cpp *.hpp)
file(GLOB LIB_TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *_test.cpp )


add_library(HRPCTestsLib STATIC)
target_link_libraries(HRPCTestsLib PRIVATE RuntimeApi Catch2::Catch2)
target_sources(HRPCTestsLib PRIVATE ${LIB_SOURCES})


add_executable(seastar_hrpc_tests_server)
target_link_libraries(seastar_hrpc_tests_server PRIVATE SeastarLib HRPCTestsLib)
target_sources(seastar_hrpc_tests_server PRIVATE seastar_hrpc_tests_server.cpp ${LIB_TEST_SOURCES})

add_executable(seastar_hrpc_tests_client)
target_link_libraries(seastar_hrpc_tests_client PRIVATE SeastarLib HRPCTestsLib)
target_sources(seastar_hrpc_tests_client PRIVATE seastar_hrpc_tests_client.cpp ${LIB_TEST_SOURCES})


add_executable(reactor_hrpc_tests_server)
target_link_libraries(reactor_hrpc_tests_server PRIVATE ReactorLib HRPCTestsLib)
target_sources(reactor_hrpc_tests_server PRIVATE reactor_hrpc_tests_server.cpp ${LIB_TEST_SOURCES})

add_executable(reactor_hrpc_tests_client)
target_link_libraries(reactor_hrpc_tests_client PRIVATE ReactorLib HRPCTestsLib)
target_sources(reactor_hrpc_tests_client PRIVATE reactor_hrpc_tests_client.cpp ${LIB_TEST_SOURCES})
