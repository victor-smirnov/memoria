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

#include "multimap_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>


namespace memoria {
namespace v1 {
namespace tests {

template <
    typename MapName
>
class MultiMapBasicTest: public MultiMapTestBase<MapName> {

    using MyType = MultiMapBasicTest<MapName>;
    using Base   = MultiMapTestBase<MapName>;


    using typename Base::Iterator;
    using typename Base::Key;
    using typename Base::Value;
    using typename Base::Ctr;


    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::getRandom;

    using Base::checkData;
    using Base::out;

    int64_t create_size               = 10000000;
    int64_t create_mean_value_size    = 100;

    int64_t upsert_size               = 10000000;
    int64_t upsert_mean_value_size    = 1024;

    int64_t remove_size               = 10000000;
    int64_t remove_mean_value_size    = 1024;

    int64_t create_coverage_size_     = 0;
    int64_t upsert_coverage_size_     = 0;
    int64_t remove_coverage_size_     = 0;

public:

    MultiMapBasicTest()
    {
    }

    MMA1_STATE_FILEDS(create_size, create_mean_value_size, upsert_size, upsert_mean_value_size, remove_size, remove_mean_value_size)

    static void init_suite(TestSuite& suite) {
        MMA1_CLASS_TESTS(suite, runCreateTest, runUpsertTest, runRemoveTest);
    }

    virtual void pre_configure(TestCoverage coverage)
    {
        switch(coverage) {
            case TestCoverage::SMOKE:
                create_coverage_size_ = 10000;
                upsert_coverage_size_ = 10000;
                remove_coverage_size_ = 10000;
                break;
            case TestCoverage::TINY:
                create_coverage_size_ = 100000;
                upsert_coverage_size_ = 100000;
                remove_coverage_size_ = 100000;
                break;
            case TestCoverage::SMALL:
                create_coverage_size_ = 1000000;
                upsert_coverage_size_ = 1000000;
                remove_coverage_size_ = 1000000;
                break;
            case TestCoverage::MEDIUM:
                create_coverage_size_ = create_size;
                upsert_coverage_size_ = upsert_size;
                remove_coverage_size_ = remove_size;
                break;
            case TestCoverage::LARGE:
                create_coverage_size_ = create_size * 10;
                upsert_coverage_size_ = upsert_size * 10;
                remove_coverage_size_ = remove_size * 10;
                break;
            case TestCoverage::XLARGE:
                create_coverage_size_ = create_size * 100;
                upsert_coverage_size_ = upsert_size * 100;
                remove_coverage_size_ = remove_size * 100;
                break;
            default:
                create_coverage_size_ = create_size;
                upsert_coverage_size_ = upsert_size;
                remove_coverage_size_ = remove_size;
        }
    }

    void runCreateTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        MultimapTestRandomBufferPopulator<Key, Value> provider(create_mean_value_size, create_coverage_size_);
        map.append_entries(provider);

        checkData(map, provider.map_data());

        for (auto& entry: provider.map_data())
        {
            auto ii = map.find(entry.first);
            AssertTrue(MA_RAW_SRC, ii != nullptr);

            ii->read_buffer();

            AssertSpansEQ(MA_RAW_SRC, ii->buffer(), Span<const Value>(entry.second));
        }

        snp.commit();
    }
    
    void runUpsertTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        MultimapTestMapDataPopulator<Key, Value> populator(upsert_mean_value_size, upsert_coverage_size_);
        populator.populate();

        auto map_data = populator.map_data();

        std::random_shuffle(map_data.begin(), map_data.end());

        for (auto& entry: map_data)
        {
            bool existing = map.upsert(
                entry.first,
                entry.second
            );

            AssertFalse(MA_RAW_SRC, existing);
        }

        checkData(map, populator.map_data());

        snp.commit();
    }
    
    
    void runRemoveTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        MultimapTestRandomBufferPopulator<Key, Value> provider(remove_mean_value_size, remove_coverage_size_);
        map.append_entries(provider);

        checkData(map, provider.map_data());

        std::map<Key, std::vector<Value>> map_data_map;

        std::vector<Key> keys;

        for (const auto& entry: provider.map_data())
        {
            keys.push_back(entry.first);
            map_data_map[entry.first] = entry.second;
        }
        
        std::random_shuffle(keys.begin(), keys.end());

        size_t cnt{};
        size_t check_size = keys.size() / 10;
        for (auto key: keys)
        {
            map.remove(key);
            map_data_map.erase(key);

            if (cnt % check_size == 0)
            {
                out() << cnt << " of " << keys.size() << std::endl;
                checkData(map, map_data_map);
            }

            cnt++;

            if (cnt + 1000 > keys.size()) {
                check_size = 1;
            }
        }

        AssertEQ(MA_RAW_SRC, map.size(), 0);

        snp.commit();
    }
};

}}}
