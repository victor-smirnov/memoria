
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/profiles/memory_cow/memory_cow_profile.hpp>
#include <memoria/api/store/memory_store_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>


#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/memoria.hpp>


#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace memoria;

using Profile = MemoryCoWProfile<>;


int main()
{
    using CtrName = AllocationMap;

    long dtrStart;

    try {
        auto store = IMemoryStore<Profile>::create().get_or_throw();

        auto master = store->master().get_or_throw();
        auto snp = master->branch().get_or_throw();

        UUID name = UUID::make_random();

        auto ctr = create(snp, CtrName{}, name).get_or_throw();

        std::cout << ctr->size().get_or_throw() << std::endl;

        auto actual = ctr->expand(1024*1024).get_or_throw();
        std::cout << ctr->size().get_or_throw() << " " << actual << std::endl;

        ctr->shrink(102400).get_or_throw();
        std::cout << ctr->size().get_or_throw() << " " << std::endl;

        ctr->mark_allocated(100000, 0, 100).get_or_throw();


        auto ii = ctr->iterator().get_or_throw();
        ii->dumpPath().get_or_throw();

        auto cnt = ii->count_fw().get_or_throw();
        std::cout << "Zeroes: " << cnt << std::endl;

        dtrStart = getTimeInMillis();
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cout);
    }
    catch (std::exception& ex) {
        std::cout << "std::exception has been thrown: " << ex.what() << std::endl;
    }
    catch (int& vv) {
        std::cout << "Int has been thrown: " << vv << std::endl;
    }

    long dtrEnd = getTimeInMillis();

    std::cout << "Dtr time: " << (dtrEnd - dtrStart) << std::endl;

    return 0;
}
