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

file(GLOB SRCS ../posix/posix_config.cpp ../posix/posix_container.cpp ../posix/posix_m*.cpp ../posix/posix_s*.cpp ../posix/posix_t*.cpp ../posix/posix_logs.cpp *.cpp)
set_property(GLOBAL APPEND PROPERTY GLOBAL_SOURCES ${SRCS})

include_directories(../posix)
