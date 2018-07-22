
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
#include <memoria/v1/api/map/map_api.hpp>

#include <memoria/v1/core/strings/format.hpp>

#include <iostream>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <random>

using namespace memoria::v1;

int main(int argc, char** argv, char** envp)
{
    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();

        using SMap = Map<U8String, U8String>;

        auto ctr = create<SMap>(snp);

        auto iter = ctr.begin();

        for (int c = 0; c < 100; c++)
        {
            iter.insert(fmt::format(u"Boo {}", c).to_u8(), fmt::format(u"Zoo {}", c).to_u8());
        }

        iter.reset();
        ctr.reset();

        snp.commit();
        snp.set_as_master();

        alloc.store("th_alloc.mma1");

    }
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
    }
}
