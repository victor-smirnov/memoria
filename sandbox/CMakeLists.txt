
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

include_directories(../tools/include)


set(MEMORIA_SANDBOX_CLASSIC ld_doc mkuid256 blk_distr)

FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX_CLASSIC})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    target_link_libraries(${MEMORIA_TARGET} Stores Containers AppInit DSLEngine)
ENDFOREACH(MEMORIA_TARGET)

set(MEMORIA_DSL_ENGINE dsl_engine_sb)
FOREACH(MEMORIA_TARGET ${MEMORIA_DSL_ENGINE})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    target_link_libraries(${MEMORIA_TARGET} DSLEngine)
ENDFOREACH(MEMORIA_TARGET)


if (BUILD_SEASTAR)
  set(MEMORIA_SANDBOX_SEASTAR seastar-sb dma)

  FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX_SEASTAR})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    target_link_libraries(${MEMORIA_TARGET} Stores Containers AppInit SeastarLib)
  ENDFOREACH(MEMORIA_TARGET)

  add_executable(seastar-corobench seastar-corobench.cpp)
  target_link_libraries(seastar-corobench SeastarLib)
  target_compile_options(seastar-corobench PUBLIC -fcoroutines)
endif()

add_executable(process_sb process_sb.cpp)
target_link_libraries(process_sb TestsRunnerLib)

add_executable(crashing crashing.cpp)

add_executable(boost-fibbench boost-fibbench.cpp)
target_link_libraries(boost-fibbench ReactorLib)
