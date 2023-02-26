
# Copyright 2023 Victor Smirnov
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


add_library(MemoriaCtrs STATIC)
target_sources(MemoriaCtrs PRIVATE ${GENERATED_MAIN_SOURCES})
target_link_libraries(MemoriaCtrs MemoriaCore)
set_target_properties(MemoriaCtrs PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
