
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

if (NOT DEFINED MEMORIA_EXAMPLES)
    set(MEMORIA_EXAMPLES map wt)
endif()

FOREACH(MEMORIA_TARGET ${MEMORIA_EXAMPLES})

    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "-g ${MEMORIA_COMPILE_FLAGS} -DMEMORIA_STATIC")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS} -luuid")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}/examples")
        
    if (NOT CHECK_SYNTAX_ONLY)
        target_link_libraries(${MEMORIA_TARGET} Memoria ${MEMORIA_LIBS})
    endif()
    
ENDFOREACH(MEMORIA_TARGET)