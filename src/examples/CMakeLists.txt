
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


MACRO (ClassicExample EX_TARGET MAIN SRCS1 SRCS2)
    message(STATUS ${MAIN} ${SRC1} ${SRCS2})

    add_executable(${EX_TARGET} ${MAIN} ${SRCS1} ${SRCS2})

    set_target_properties(${EX_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMMA1_NO_REACTOR")
    set_target_properties(${EX_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    target_link_libraries(${EX_TARGET} MemoriaCtrs MemoriaStores MemoriaCore MemoriaInit ${MEMORIA_LIBS})
ENDMACRO()

MACRO (Example EX_TARGET MAIN SRCS1 SRCS2)
    message(STATUS ${MAIN} ${SRC1} ${SRCS2})

    add_executable(${EX_TARGET} ${MAIN} ${SRCS1} ${SRCS2})

    set_target_properties(${EX_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${EX_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    target_link_libraries(${EX_TARGET} MemoriaCtrsReactor MemoriaStoresReactor MemoriaCoreReactor ${MEMORIA_LIBS})
ENDMACRO()

if (BUILD_CLASSIC_MEMORIA_LIB)
    ClassicExample(datatypes datatypes.cpp "" "")

    if (BUILD_MEMORY_STORE)
        if (BUILD_CONTAINERS)
            ClassicExample(mapped mapped.cpp "" "")
            ClassicExample(map_example map_example.cpp "" "")
            ClassicExample(set_example set_example.cpp "" "")
            ClassicExample(ctrprops_example ctrprops_example.cpp "" "")
            ClassicExample(multimap_example multimap_example.cpp "" "")
            ClassicExample(store store.cpp "" "")
            ClassicExample(vector_example vector_example.cpp "" "")
            ClassicExample(sdn2_example sdn2_example.cpp "" "")
        else()
            if (BUILD_CONTAINERS_MULTIMAP)
                ClassicExample(multimap_example multimap_example.cpp "" "")
            endif()
            if (BUILD_CONTAINERS_SET)
                ClassicExample(set_example set_example.cpp "" "")
                ClassicExample(set_pointq_example set_pointq_example.cpp "" "")
                ClassicExample(set_pointq_example2 set_pointq_example2.cpp "" "")
            endif()
        endif()
    endif()
endif()

