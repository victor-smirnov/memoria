
# Copyright 2018 Victor Smirnov
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

SET(MEMORIA_APPS tcp_echo_server tcp_echo_client asio_echo_server blocking_tcp_echo_client)

FOREACH(MEMORIA_TARGET ${MEMORIA_APPS})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    SET_TARGET_PROPERTIES(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(${MEMORIA_TARGET} Memoria ${MEMORIA_LIBS})
ENDFOREACH()


