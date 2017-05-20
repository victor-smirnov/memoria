
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

set(MEMORIA_EXAMPLES map vector)

set (CTR_INST lib/map_int64_string_dflt.cpp)
set (CTR_INST ${CTR_INST} lib/vector_int_dflt.cpp)
set (CTR_INST ${CTR_INST} lib/vector_string_dflt.cpp)


MACRO (Example EX_TARGET MAIN SRCS1 SRCS2)
    message(STATUS ${MAIN} ${SRC1} ${SRCS2})

    add_executable(${EX_TARGET} ${MAIN} ${SRCS1} ${SRCS2})
    
    set_target_properties(${EX_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${EX_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    
    if (BUILD_MSVC)
        target_link_libraries(${EX_TARGET} Memoria ${MEMORIA_LIBS})
    else()
        target_link_libraries(${EX_TARGET} Memoria boost_context boost_system boost_filesystem boost_program_options m ${MEMORIA_LIBS})
    endif()
    
ENDMACRO()


Example(map map.cpp lib/map_int64_string_dflt.cpp "")

Example(vector vector.cpp lib/vector_int_dflt.cpp lib/vector_string_dflt.cpp)

Example(multimap multimap.cpp lib/multimap_uuid_byte_dflt.cpp "")
