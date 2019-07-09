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

#include "../prototype/bt/bt_test_base.hpp"

#include "multimap_populator.hpp"

#include <memoria/v1/api/multimap/multimap_api.hpp>

#include <memoria/v1/tests/tools.hpp>

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

protected:
    static constexpr int32_t DataStreams = Ctr::DataStreams;
    

    using DataSizesT = core::StaticVector<CtrSizeT, DataStreams>;

    int64_t size               = 10000000;
    int64_t mean_value_size    = 1024;

    int64_t coverage_  = 0;

public:
    using Base::getRandom;

    MultiMapTestBase()
    {}

    MMA1_STATE_FILEDS(size, mean_value_size)

    virtual void pre_configure(TestCoverage coverage)
    {
        switch(coverage) {
            case TestCoverage::SMOKE:  coverage_ = 10000; break;
            case TestCoverage::TINY:   coverage_ = 100000; break;
            case TestCoverage::SMALL:  coverage_ = 1000000; break;
            case TestCoverage::MEDIUM: coverage_ = size; break;
            case TestCoverage::LARGE:  coverage_ = size * 10; break;
            case TestCoverage::XLARGE: coverage_ = size * 100; break;
            default: coverage_ = size;
        }
    }

    void checkData(Ctr& ctr, const MapData<Key, Value>& data)
    {
        AssertEQ(MA_RAW_SRC, ctr.size(), data.size());

        size_t c = 0;
        auto iter = ctr.seek(0);

        iter->for_each_buffered([&](auto key, auto values){
            AssertEQ(MA_RAW_SRC, key, data[c].first);
            AssertSpansEQ(MA_RAW_SRC, values, Span<const Value>(data[c].second));
            c++;
        });
    }
};

}}}
