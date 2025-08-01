
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


set(MEMORIA_SANDBOX_CLASSIC swmr_store simple_set)


FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX_CLASSIC})
  add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
  set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
  set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
  target_link_libraries(${MEMORIA_TARGET} AppInit ${MEMORIA_LIBS})

  install(TARGETS
    ${MEMORIA_TARGET}
    EXPORT MemoriaTargets
    RUNTIME DESTINATION ${MEMORIA_TOOLS_INSTALL_DIR}
  )
ENDFOREACH(MEMORIA_TARGET)

if (BUILD_SEASTAR)
  set(MEMORIA_SANDBOX_SEASTAR)

  FOREACH(MEMORIA_TARGET ${MEMORIA_SANDBOX_SEASTAR})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    set_target_properties(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(${MEMORIA_TARGET} AppInit SeastarLib)

    install(TARGETS
      ${MEMORIA_TARGET}
      EXPORT MemoriaTargets
      RUNTIME DESTINATION ${MEMORIA_TOOLS_INSTALL_DIR}
    )
  ENDFOREACH(MEMORIA_TARGET)
endif()
