
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

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

int main()
{
    try {
        using MapType = Set<Varchar>;

        auto alloc = create_memory_store().get_or_throw();

        auto snp = alloc->master().get_or_throw()->branch().get_or_throw();

        auto ctr0 = create(snp, MapType()).get_or_throw();

        //ctr0->set_new_block_size(64*1024);

        int64_t t0 = getTimeInMillis();

        std::vector<U8String> entries_;

        for (int c = 0; c < 1000000; c++) {
            entries_.emplace_back(
                "AAAAAAAAAAAAAAAAAAAAAAAAAAA_"   + std::to_string(c)
            );
        }

        std::sort(entries_.begin(), entries_.end(), [](const auto& one, const auto& two){
            return one < two;
        });

        int64_t t1 = getTimeInMillis();

        std::cout << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        int64_t t0_i = getTimeInMillis();

        ctr0->append([&](auto& keys, size_t batch_start) {

            size_t batch_size = 8192;
            size_t limit = (batch_start + batch_size <= entries_.size()) ? batch_size : entries_.size() - batch_start;

            for (size_t c = 0; c < limit; c++) {
                keys.append(entries_[batch_start + c]);
            }

            return limit != batch_size;
        }).throw_if_error();


        int64_t t1_i = getTimeInMillis();

        std::cout << "Inserted entries in " << (t1_i - t0_i) << " ms" << std::endl;
        std::cout << "Size = " << ctr0->size() << std::endl;

        ctr0->iterator().get_or_throw()->dumpPath().get_or_throw();

        snp->commit().throw_if_error();
        snp->set_as_master().throw_if_error();

        alloc->store("store-set.mma1").throw_if_error();

        int64_t t2 = getTimeInMillis();

        auto ii = ctr0->scanner();

        size_t sum0 = 0;

        while (!ii.is_end())
        {
            sum0 += ii.keys().size();

            auto keys = ii.keys();

            for (size_t c = 0; c < keys.size(); c++)
            {
                std::cout << keys[c] << std::endl;
            }

            ii.next_leaf().throw_if_error();
        }

        int64_t t3 = getTimeInMillis();

        std::cout << "Iterated over 10M entries in " << (t3 - t2) << " ms " << sum0 << std::endl;

        for (int64_t c = 0; c < ctr0->size().get_or_throw(); c++)
        {
            U8String key = "AAAAAAAAAAAAAAAAAAAAAAAAAAA_" + std::to_string(c);

            auto ii = ctr0->find(key).get_or_throw();

            if (!ii->is_end())
            {
                std::cout << key << " :: " << ii->key().view() << std::endl;
            }
        }
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cerr);
    }

    return 0;
}
