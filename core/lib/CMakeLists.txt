
# Copyright 2016-2023 Victor Smirnov
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

file(GLOB_RECURSE LIB_SOURCES *.cpp *.hpp hermes/*.cpp platforms/posix/*.cpp)

add_library(MemoriaCore STATIC )
target_include_directories(MemoriaCore PUBLIC ../../include)
target_include_directories(MemoriaCore PUBLIC include)

SET_TARGET_PROPERTIES(MemoriaCore PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
set_property(TARGET MemoriaCore PROPERTY CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})
target_link_libraries(MemoriaCore absl::base absl::strings absl::hash absl::meta absl::time)
target_link_libraries(MemoriaCore fmt::fmt unofficial::hash-library)

file(GLOB_RECURSE LIB_HEADERS "include/*.*" "../../include/*.*")
target_sources(
    MemoriaCore
    PRIVATE ${LIB_SOURCES}
    PUBLIC ${LIB_HEADERS}
)

