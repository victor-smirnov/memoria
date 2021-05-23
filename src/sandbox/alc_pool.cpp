
// Copyright 2021 Victor Smirnov
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

#include <memoria/store/swmr/common/allocation_pool.hpp>
#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>


#include <iostream>

using namespace memoria;

using Profile = ApiProfile<CowLiteProfile<>>;

using AlcPool = AllocationPool<Profile, 7>;
using AlcMeta = AllocationMetadata<Profile>;

int main(void) {
    try {
        AlcPool pool;

        AlcMeta meta(20, 1, 2);

        pool.add(meta);

        pool.dump();

        auto alc1 = pool.allocate_one(0).get();
//        auto alc2 = pool.allocate_one(0).get();
//        auto alc3 = pool.allocate_one(0).get();
//        auto alc4 = pool.allocate_one(0).get();

        //println("{} {} {} {}", alc1, alc2, alc3, alc4);

        println("{}", alc1);

        pool.dump();
    }
    catch (const MemoriaError& ee) {
        ee.describe(std::cout);
    }
    catch (const MemoriaThrowable& ee) {
        ee.dump(std::cout);
    }
    catch (const std::exception& ee) {
        std::cerr << "Exception: " << ee.what() << std::endl;
    }
}
