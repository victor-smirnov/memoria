// Copyright 2012 Victor Smirnov
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


#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {
namespace tests {

template <
    typename MapName
>
class MapCreateTest: public MapTestBase<MapName> {

    using MyType = MapCreateTest<MapName>;
    using Base   = MapTestBase<MapName>;


    using typename Base::Allocator;
    using typename Base::Iterator;
    using typename Base::Pair;
    using typename Base::PairVector;
    using typename Base::Key;
    using typename Base::Value;

    using Base::size_;
    using Base::ctr_name_;
    using Base::vector_idx_;
    using Base::pairs;
    using Base::pairs_sorted;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::checkContainerData;
    using Base::out;
    using Base::ctr_check_iterator;


    Key     key_;
    Value   value_;

public:

    MapCreateTest()
    {
        Base::size_ = 10000;
    }

    MMA_STATE_FILEDS(key_, value_)

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TEST_WITH_REPLAY(suite, runCreateTest, replayCreateTest);
    }


    virtual Key makeRandomKey() {
        return Key::make_random();
    }

    virtual Value makeRandomValue() {
        return this->getBIRandom();
    }


    void runCreateTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        map.new_block_size(4096);

        ctr_name_ = map.name();

        for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
        {
            out() << vector_idx_ << std::endl;

            auto key    = pairs[vector_idx_].key_;
            auto value  = pairs[vector_idx_].value_;

            {
                auto iter = map.assign(key, value);

                ctr_check_iterator(iter, MA_SRC);
            }

            check(snp, MA_SRC);

            PairVector tmp = pairs_sorted;

            appendToSortedVector(tmp, pairs[vector_idx_]);

            checkContainerData(map, tmp);

            commit();

            snp = branch();
            map = find<MapName>(snp, ctr_name_);

            pairs_sorted = tmp;
        }

        if (snapshot().is_active())
        {
            commit();
        }
    }

    void replayCreateTest()
    {
        auto snp = branch();
        auto map = find<MapName>(snp, ctr_name_);

        auto key = pairs[vector_idx_].key_;
        auto value = pairs[vector_idx_].value_;

        {
            auto iter = map.assign(key, value);

            ctr_check_iterator(iter, MA_SRC);
        }

        check(snp, MA_SRC);

        appendToSortedVector(pairs_sorted, pairs[vector_idx_]);

        checkContainerData(map, pairs_sorted);

        commit();
    }
};

}}
