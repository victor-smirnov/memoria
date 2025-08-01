
# Copyright 2016-2025 Victor Smirnov
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

find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(unofficial-hash-library CONFIG REQUIRED)
find_package(ICU 60.2 COMPONENTS i18n uc data REQUIRED)

file(GLOB_RECURSE LIB_SOURCES lib/*.cpp lib/*.hpp lib/hermes/*.cpp lib/platforms/posix/*.cpp)

target_include_directories(Core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

set_target_properties(Core PROPERTIES CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})
set_target_properties(Core PROPERTIES OUTPUT_NAME memoria_core)

target_link_libraries(Core PUBLIC absl::base absl::strings absl::hash absl::meta absl::time)
target_link_libraries(Core PUBLIC fmt::fmt)
target_link_libraries(Core PUBLIC Boost::filesystem)
target_link_libraries(Core PUBLIC ICU::i18n ICU::uc ICU::data)

target_link_libraries(Core PRIVATE unofficial::hash-library)

target_compile_definitions(Core PUBLIC  -DMMA_ICU_CXX_NS=icu)
target_compile_definitions(Core PRIVATE -DMEMORIA_SRC=${CMAKE_SOURCE_DIR})
target_compile_definitions(Core PUBLIC  -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)

target_compile_options(Core PUBLIC -pthread)
target_compile_options(Core PUBLIC -march=native -fstrict-aliasing -Wstrict-aliasing=2)


if (USE_ASAN)
    target_compile_definitions(Core PUBLIC -DBOOST_USE_ASAN -DBOOST_USE_UCONTEXT)
    target_compile_options(Core PUBLIC -fsanitize=address -fsanitize-address-use-after-scope)
    target_link_options(Core PUBLIC -fsanitize=address)
endif()

if (USE_USAN)
    target_compile_options(Core PUBLIC -fsanitize=undefined)
    target_link_options(Core PUBLIC -fsanitize=undefined)
endif()

if (USE_TSAN)
    target_compile_definitions(Core PUBLIC -DBOOST_USE_TSAN -DBOOST_USE_UCONTEXT)
    target_compile_options(Core PUBLIC -fsanitize=thread)
    target_link_options(Core PUBLIC -fsanitize=thread)
endif()

file(GLOB_RECURSE LIB_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "include/*.*" )

target_sources(
  Core
  PRIVATE ${LIB_SOURCES}
  PRIVATE ${LIB_HEADERS}
)

install(TARGETS Core EXPORT MemoriaTargets)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}
)
