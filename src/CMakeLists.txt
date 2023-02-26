
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


include_directories(../include)
include_directories(core/include)
include_directories(../generated)
include_directories(../codegen)

add_subdirectory(app_init)



get_property(GLOBAL_INIT_SRCS GLOBAL PROPERTY GLOBAL_INIT_SOURCES)
add_library(MemoriaInit STATIC)
target_sources(MemoriaInit PRIVATE ${GLOBAL_INIT_SRCS})
target_link_libraries(MemoriaInit MemoriaCore MemoriaCtrs MemoriaStores)

set_property(TARGET MemoriaInit MemoriaCtrs MemoriaStores PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

