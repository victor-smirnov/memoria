
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

#include <memoria/profiles/core_api/core_api_profile.hpp>
#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

int main()
{
    try {

        using VectorType = Vector<Varchar>;

        auto alloc = create_memory_store().get_or_throw();

        auto snp = alloc->master().get_or_throw()->branch().get_or_throw();

        auto ctr0 = create(snp, VectorType()).get_or_throw();

        //ctr0->set_new_block_size(64*1024);

        int64_t t0 = getTimeInMillis();

        std::vector<U8String> entries;

        for (int c = 0; c < 10000; c++) {
            entries.emplace_back("AAAAAAAAAAAAAAAAAAAAAAAAAAA_" + std::to_string(c));
        }

        int64_t t1 = getTimeInMillis();

        std::cout << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        int64_t t0_i = getTimeInMillis();


        ctr0->append([&](auto& values, size_t batch_start) {

            size_t batch_size = 8192 * 32;
            size_t limit = (batch_start + batch_size <= entries.size()) ? batch_size : entries.size() - batch_start;

            values.append(Span<const U8String>(entries.data() + batch_start, limit));

            return limit != batch_size;
        }).throw_if_error();


        int64_t t1_i = getTimeInMillis();

        std::cout << "Inserted entries in " << (t1_i - t0_i) << " ms" << std::endl;
        std::cout << "Size = " << ctr0->size() << std::endl;

//        ctr0->seek(0)->dumpPath();

        snp->commit().throw_if_error();
        snp->set_as_master().throw_if_error();

        alloc->store("store-vector.mma1").throw_if_error();

        int64_t t2 = getTimeInMillis();
        size_t sum0 = 0;

        auto scanner = ctr0->as_scanner([](auto ctr){
            return ctr->seek(0).get_or_throw();
        });

        while (!scanner.is_end())
        {
            for (const auto& vv: scanner.values()) {
                std::cout << vv << std::endl;
            }

            scanner.next_leaf().throw_if_error();
        }

        int64_t t3 = getTimeInMillis();

        std::cout << "Iterated over " << ctr0->size() << " entries in " << (t3 - t2) << " ms " << sum0 << std::endl;
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cerr);
    }

    return 0;
}
