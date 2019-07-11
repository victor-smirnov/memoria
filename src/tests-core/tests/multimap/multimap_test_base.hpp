// Copyright 2016 Victor Smirnov
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


#pragma once

#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/tests/tools.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include "multimap_populator.hpp"
#include "multimap_map_data.hpp"


#include <vector>
#include <algorithm>
#include <sstream>
#include <tuple>
#include <map>

namespace memoria {
namespace v1 {
namespace tests {

template<typename MapName>
class MultiMapTestBase: public BTTestBase<MapName, InMemAllocator<>, DefaultProfile<>> {
    using MyType    = MultiMapTestBase<MapName>;
    using Base      = BTTestBase<MapName, InMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::Iterator;

    using CtrSizesT = typename Ctr::CtrSizesT;
    using CtrSizeT  = typename Ctr::CtrSizeT;

    using Base::out;

    using Key       = typename Ctr::Key;
    using Value     = typename Ctr::Value;

public:
    using Base::getRandom;

    MultiMapTestBase()
    {}





    void checkData(Ctr& ctr, const MapData<Key, Value>& data)
    {
        AssertEQ(MA_RAW_SRC, ctr.size(), data.size());

        size_t c = 0;
        auto iter = ctr.seek(0);

//        iter->for_each_buffered([&](auto key, auto values){
//            AssertEQ(MA_RAW_SRC, key, data[c].first);
//            AssertSpansEQ(MA_RAW_SRC, values, Span<const Value>(data[c].second));
//            c++;
//        });

        iter->set_buffered();

        while (!iter->is_end())
        {
            if ((!iter->is_first_iteration()) && iter->is_buffer_ready())
            {
                AssertEQ(MA_RAW_SRC, iter->suffix_key(), data[c].first);
                AssertSpansEQ(MA_RAW_SRC, iter->buffer(), Span<const Value>(data[c].second));
                c++;
            }

            if (iter->has_entries())
            {
                for (auto& entry: iter->entries())
                {
                    AssertEQ(MA_RAW_SRC, *entry.key, data[c].first);
                    AssertSpansEQ(MA_RAW_SRC, entry.values, Span<const Value>(data[c].second));
                    c++;
                }
            }

            iter->next();
        }
    }


    void checkData(Ctr& ctr, const std::map<Key, std::vector<Value>>& data)
    {
        AssertEQ(MA_RAW_SRC, ctr.size(), data.size());

        size_t c = 0;
        auto iter = ctr.seek(0);

        iter->for_each_buffered([&](auto key, auto values){

            auto ii = data.find(key);

            AssertTrue(MA_RAW_SRC, ii != data.end());
            AssertEQ(MA_RAW_SRC, key, ii->first);
            AssertSpansEQ(MA_RAW_SRC, values, Span<const Value>(ii->second));
            c++;
        });
    }

};

}}}
