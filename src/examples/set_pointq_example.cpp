
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

#include <memoria/profiles/default/default.hpp>
#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;



int main()
{
    StaticLibraryCtrs<>::init();

    try {
        using MapType = Set<Varchar>;

        auto alloc = IMemoryStore<>::create().get_or_throw();

        auto snp = alloc->master().get_or_throw()->branch().get_or_throw();

        auto ctr0 = create(snp, MapType()).get_or_throw();

        ctr0->set_new_block_size(4096).get_or_throw();

        int64_t t0 = getTimeInMillis();

        std::vector<U8String> entries_;

        for (int c = 0; c < 1000000; c++) {
            if (c % 10000 == 0) {
                std::cout << "C=" << c << std::endl;
            }

            U8String key = create_random_string(10); //U8String("Key____") + toString(c)

            ctr0->insert(key).get_or_throw();
        }

        //ctr0->iterator().get_or_throw()->dumpPath(std::cout);

        int64_t t1 = getTimeInMillis();

        std::cout << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        snp->commit().get_or_throw();
        alloc->store("set_insq.mma1").get_or_throw();
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cerr);
    }
    catch (ResultException& th) {
        std::cout << "[" << th.what() << "]" << std::endl;
    }

    return 0;
}
