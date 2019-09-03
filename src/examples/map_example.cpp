
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


#include <memoria/v1/api/datatypes/datatypes.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/memoria.hpp>

#include <iostream>

using namespace memoria::v1;

int main()
{
    StaticLibraryCtrs<>::init();

//    using MapType = Map<BigInt, BigInt>;
//    using Entry   = std::pair<int64_t, int64_t>;

    using MapType = Map<Varchar, Varchar>;
    using Entry   = std::pair<U8String, U8String>;

//    using MapType = Map<BigInt, Varchar>;
//    using Entry   = std::pair<int64_t, U8String>;

    auto alloc = IMemoryStore<>::create();

    auto snp = alloc->master()->branch();

    auto ctr0 = snp->create_ctr(MapType());

    //ctr0->set_new_block_size(64*1024);

    int64_t t0 = getTimeInMillis();

    std::vector<Entry> entries_;

    for (int c = 0; c < 110; c++) {
        //entries_.emplace_back(Entry(c, -c));
        entries_.emplace_back(Entry(
            //c,
            "AAAAAAAAAAAAAAAAAAAAAAAAAAA_" + std::to_string(c),
            "BBBBBBBBBBBBBBBBBBBBBBBBBBBBB_" + std::to_string(c)
           // c, -c
        ));
    }

    int64_t t1 = getTimeInMillis();

    std::cout << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

    int64_t t0_i = getTimeInMillis();

    ctr0->append_entries([&](auto& keys, auto& values, size_t batch_start) {

        size_t batch_size = 8192;
        size_t limit = (batch_start + batch_size <= entries_.size()) ? batch_size : entries_.size() - batch_start;

        for (size_t c = 0; c < limit; c++) {
            keys.append(std::get<0>(entries_[batch_start + c]));
            values.append(std::get<1>(entries_[batch_start + c]));
        }

        return limit != batch_size;
    });

    int64_t t1_i = getTimeInMillis();

    std::cout << "Inserted entries in " << (t1_i - t0_i) << " ms" << std::endl;
    std::cout << "Size = " << ctr0->size() << std::endl;

    ctr0->iterator()->dump();

    snp->commit();
    snp->set_as_master();


    alloc->store("store.mma1");

    int64_t t2 = getTimeInMillis();

    auto ii = ctr0->scanner();

    size_t sum0 = 0;


    while (!ii.is_end())
    {
        sum0 += ii.keys().size() + ii.values().size();

        auto keys = ii.keys();

        for (size_t c = 0; c < keys.size(); c++)
        {
            std::cout << keys[c] << " = " << ii.values()[c] << std::endl;
        }

        ii.next_leaf();
    }

    int64_t t3 = getTimeInMillis();

    std::cout << "Iterated over 10M entries in " << (t3 - t2) << " ms " << sum0 << std::endl;

    return 0;
}
