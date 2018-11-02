
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
#include <memoria/v1/api/map/map_api.hpp>

#include <memoria/v1/core/tools/uuid.hpp>


#include <iostream>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <random>

#include <thread>

using namespace memoria::v1;

using MapType = Map<U8String, U8String>;

int main(int argc, char** argv, char** envp)
{
    try {
        auto alloc = ThreadInMemAllocator<>::create();

        auto snp = alloc.master().branch();
        auto ctr = create<MapType>(snp);

        auto stat = snp.memory_stat();

        std::cout << "Ctr: " << ctr.name() << std::endl;

        snp.commit();

        auto snp2 = snp.branch();

        auto ctr2 = find<MapType>(snp2, ctr.name());

        for (int c = 0; c < 500000; c++) {
            ctr2.assign(U8String("AAA") + std::to_string(c), "BBB");
        }

        snp2.commit();

        auto stat2 = alloc.memory_stat();

        print(std::cout, *stat2.get());
        std::cout << "\n";

        std::cout << "Done..." << std::endl;

        alloc.store("alloc.mma1");
    }
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
    }
}
