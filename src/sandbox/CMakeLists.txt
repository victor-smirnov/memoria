
# Copyright 2016 Victor Smirnov
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

if (BUILD_REACTOR_MEMORIA_LIB)

    if (NOT DEFINED MEMORIA_SANDBOX)
        set(MEMORIA_SANDBOX)
    endif()

    FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX})
        add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        target_link_libraries(${MEMORIA_TARGET} MemoriaCtrsReactor MemoriaStoresReactor MemoriaCoreReactor MemoriaInitReactor ${MEMORIA_LIBS})

        target_link_libraries(${MEMORIA_TARGET} ${MEMORIA_LIBS})
    ENDFOREACH(MEMORIA_TARGET)

endif()


if (BUILD_CLASSIC_MEMORIA_LIB)

    if (NOT DEFINED MEMORIA_SANDBOX_CLASSIC)
        set(MEMORIA_SANDBOX_CLASSIC swmr_store)

        if (NO_COW_PROFILE)
            set(MEMORIA_SANDBOX_CLASSIC ${MEMORIA_SANDBOX_CLASSIC})
        endif()
    endif()

    FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX_CLASSIC})
        add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMMA1_NO_REACTOR")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        target_link_libraries(${MEMORIA_TARGET} MemoriaCtrs MemoriaStores MemoriaCore MemoriaInit ${MEMORIA_LIBS})

        target_link_libraries(${MEMORIA_TARGET} ${MEMORIA_LIBS})
    ENDFOREACH(MEMORIA_TARGET )

endif()
