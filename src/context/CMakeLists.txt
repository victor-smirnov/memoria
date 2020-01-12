# Copyright 2017 Victor Smirnov
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


file(GLOB SRCS posix/*.cpp fiber.cpp asm/jump_x86_64_sysv_elf_gas.S asm/make_x86_64_sysv_elf_gas.S asm/ontop_x86_64_sysv_elf_gas.S ${EXTRA_SRC})

set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})

