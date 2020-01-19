
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memoria/core/types/type2type.hpp>
#include <memoria/core/tools/hash.hpp>
#include <memoria/core/container/logs.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif


namespace memoria {

int64_t DebugCounter = 0;
int64_t DebugCounter1 = 0;
int64_t DebugCounter2 = -1;

LogHandler* Logger::default_handler_ = new DefaultLogHandlerImpl();
Logger logger("Memoria", Logger::INFO, NULL);

}
