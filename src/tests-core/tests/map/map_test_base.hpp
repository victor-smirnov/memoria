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

#include "../prototype/btss/btss_test_base.hpp"

#include <memoria/v1/api/map/map_api.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

namespace _ {

template <typename T>
struct RNGTool {
    template <typename Test>
    static T next(Test* test) {
        return test->getBIRandom();
    }
};


template <>
struct RNGTool<UUID> {
    template <typename Test>
    static UUID next(Test* test) {
        return UUID::make_random();
    }
};

}

namespace tests {

template <typename K, typename A>
struct IndirectStateFiledSerializer<std::vector<K, A>> {
    static void externalize(std::vector<K, A>& field, const filesystem::path& path, ConfigurationContext* context) {
        StoreVector(field, path.to_u16());
    }

    static void internalize(std::vector<K, A>& field, const filesystem::path& path, ConfigurationContext* context) {
        LoadVector(field, path.to_u16());
    }
};


template<typename MapName>
class MapTestBase: public BTSSTestBase<MapName, InMemAllocator<>, DefaultProfile<>> {
    using MyType = MapTestBase<MapName>;
    using Base = BTSSTestBase<MapName, InMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::Iterator;
    using typename Base::MemBuffer;
    using typename Base::DataValue;

    using Base::out;

    using Key       = typename Ctr::Key;
    using Value     = typename Ctr::Value;
    using Entry     = typename Ctr::DataValue;

    using Pair = KVPair<Key, Value>;

protected:
    typedef std::vector<Pair> PairVector;

    PairVector pairs;
    PairVector pairs_sorted;

    int32_t vector_idx_;
    int32_t step_;

    UUID ctr_name_;

    int32_t iterator_check_count_ = 100;
    int32_t iterator_check_counter_ = 0;

    int32_t data_check_count_ = 100;
    int32_t data_check_counter_ = 0;

    int64_t size_{10000};

    bool throw_ex_ = false;

public:

    MapTestBase()
    {
    }

    MMA1_STATE_FILEDS(size_, throw_ex_, vector_idx_,
                      step_, ctr_name_,
                      iterator_check_counter_,
                      iterator_check_count_,
                      data_check_counter_,
                      data_check_count_)

    MMA1_INDIRECT_STATE_FILEDS(pairs, pairs_sorted)

    Key getRandomKey()
    {
        return _::RNGTool<Key>::next(this);
    }

    Value getRandomValue()
    {
        return _::RNGTool<Value>::next(this);
    }

    virtual MemBuffer createRandomBuffer(int32_t size)
    {
        auto buffer = MemBuffer(size);

        for (auto& v: buffer)
        {
            v = Entry(getRandomKey(), getRandomValue());
        }

        return buffer;
    }


    template <typename CtrT>
    void checkContainerData(CtrT& map, PairVector& pairs)
    {
        if (data_check_counter_ % data_check_count_ == 0)
        {
            int32_t pairs_size = (int32_t) pairs.size();

            int32_t idx = 0;
            for (auto iter = map.begin(); !iter.is_end();)
            {
                auto key = iter.key();

                auto value = iter.value();

                if (pairs[idx].key_ != key) {
                    iter.dump();
                }

                if (pairs[idx].value_ != value) {
                    iter.dump();
                }

                assert_equals(pairs[idx].key_, key);
                assert_equals(pairs[idx].value_, value);

                iter.next();
                idx++;
            }

            assert_equals(idx, pairs_size);
        }

        data_check_counter_++;
    }

    virtual void checkIterator(Iterator& iter, const char* source) {
        iter.check(out(), source);
    }

    virtual Key makeRandomKey() = 0;
    virtual Value makeRandomValue() = 0;


    virtual void set_up() noexcept
    {
        Base::set_up();

        if (!this->isReplayMode())
        {
            pairs.clear();
            pairs_sorted.clear();

            for (int32_t c = 0; c < size_; c++)
            {
                pairs.push_back(Pair(makeRandomKey(), makeRandomValue()));
            }
        }
    }
};

}}}
