
# Copyright 2021 Victor Smirnov
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



pybind11_add_module(memoria python_bindings.cpp)
target_link_libraries(memoria PRIVATE MemoriaCtrs MemoriaStores MemoriaCore MemoriaInit pybind11::lto pybind11::embed pybind11::module)
target_compile_definitions(memoria PRIVATE VERSION_INFO=${MEMORIA_VERSION_INFO})
