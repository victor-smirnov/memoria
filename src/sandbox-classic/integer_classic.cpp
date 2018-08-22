
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

#include <memoria/v1/api/multimap/multimap_api.hpp>

#include <memoria/v1/core/tools/uuid.hpp>


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

        auto snp1 = alloc.master().branch();

        auto name = UUID::make_random();

        auto ctr1 = create<Multimap<int64_t, uint8_t>>(snp1, name);
        auto ctr2 = find<Multimap<int64_t, uint8_t>>(snp1, name);

        std::cout << ctr1.ptr() << " :: " << ctr2.ptr() << std::endl;

        ctr2.cleanup();

        std::vector<uint8_t> data(100);

        //ctr1.assign(1, data.begin(), data.end());

        snp1.commit();
        std::cout << "Active snapshots num: " << alloc.active_snapshots() << std::endl;
        alloc.store("allocator.mma1", 3000);
    }
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
    }
}
