
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


set (CTR_INST lib/map_int64_string_dflt.cpp)
set (CTR_INST ${CTR_INST} lib/vector_int_dflt.cpp)
set (CTR_INST ${CTR_INST} lib/vector_ubyte_dflt.cpp)
set (CTR_INST ${CTR_INST} lib/vector_string_dflt.cpp)


MACRO (Example EX_TARGET MAIN SRCS1 SRCS2)
    message(STATUS ${MAIN} ${SRC1} ${SRCS2})

    add_executable(${EX_TARGET} ${MAIN} ${SRCS1} ${SRCS2})
    
    set_target_properties(${EX_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMMA1_NO_REACTOR")
    set_target_properties(${EX_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    
    target_link_libraries(${EX_TARGET} MemoriaClassic ${MEMORIA_LIBS})
ENDMACRO()


#Example(map map.cpp lib/map_int64_string_dflt.cpp "")

#Example(vector vector.cpp lib/vector_int_dflt.cpp lib/vector_string_dflt.cpp)

#Example(multimap multimap.cpp lib/multimap_uuid_byte_dflt.cpp "")
#Example(multimap_reader multimap_reader.cpp lib/multimap_uuid_byte_dflt.cpp "")
#Example(multimap_iov_all multimap_iov_all.cpp "" "") #lib/multimap_uuid_byte_dflt.cpp
#Example(multimap_stream multimap_stream.cpp lib/multimap_uuid_uint64_dflt.cpp "")
#Example(multimap_iovector multimap_iovector.cpp lib/multimap_uuid_uint64_dflt.cpp "")
#Example(vector_iovector_ubyte vector_iovector_ubyte.cpp lib/vector_ubyte_dflt.cpp "")
#Example(vector_iovector_string vector_iovector_string.cpp lib/vector_string_dflt.cpp "")

#Example(map_iovector_ss map_iovector_ss.cpp lib/vector_string_dflt.cpp "")
#Example(map_iovector_ss_lw map_iovector_ss_lw.cpp lib/vector_string_dflt.cpp "")


Example(typesig typesig.cpp "" "")

Example(store store.cpp "" "")
