
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/reactor/reactor.hpp>

namespace memoria {

std::vector<MemoryObjectList>& MemoryObjectList::object_lists() {
    int cpus = reactor::engine().cpu_num();
    static thread_local std::vector<MemoryObjectList> lists(cpus);
    return lists;
}

void MemoryObjectList::link(MemoryObject* msg)
{
    int cpu = reactor::engine().cpu();
    MemoryObjectList& list = object_lists()[cpu];
    msg->next_ = list.head_;
    list.head_ = msg;
    list.size_++;
}

}
