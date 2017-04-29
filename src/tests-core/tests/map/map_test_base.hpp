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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/api/map_api.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "../prototype/btss/btss_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

namespace {

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


template <typename Key, typename Value>
struct IOBufferAdapter<std::tuple<Key, Value>> {

    using T = std::tuple<Key, Value>;

    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const T& value)
    {
        if (IOBufferAdapter<Key>::put(buffer, std::get<0>(value)))
        {
            if (IOBufferAdapter<Value>::put(buffer, std::get<1>(value))) {
                return true;
            }
        }

        return false;
    }

    template <typename IOBuffer>
    static T get(IOBuffer& buffer)
    {
        return std::make_tuple(IOBufferAdapter<Key>::get(buffer), IOBufferAdapter<Value>::get(buffer));
    }
};


template<typename MapName>
class MapTestBase: public BTSSTestBase<MapName, ThreadInMemAllocator<>, DefaultProfile<>> {
    using MyType = MapTestBase<MapName>;
    using Base = BTSSTestBase<MapName, ThreadInMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::Iterator;
    using typename Base::MemBuffer;
    using typename Base::Entry;

    using Base::out;

    using Key       = typename Ctr::Types::Key;
    using Value     = typename Ctr::Types::Value;

    using Pair = KVPair<Key, Value>;

protected:
    typedef vector<Pair> PairVector;

    PairVector pairs;
    PairVector pairs_sorted;

    Int vector_idx_;
    Int step_;

    UUID ctr_name_;

    String pairs_data_file_;
    String pairs_sorted_data_file_;

    Int iterator_check_count_ = 100;
    Int iterator_check_counter_ = 0;

    Int data_check_count_ = 100;
    Int data_check_counter_ = 0;

    BigInt size_;

    bool throw_ex_ = false;

public:

    MapTestBase(StringRef name): Base(name)
    {
        size_ = 10000;


        MEMORIA_ADD_TEST_PARAM(size_);
        MEMORIA_ADD_TEST_PARAM(throw_ex_);

        MEMORIA_ADD_TEST_PARAM(vector_idx_)->state();
        MEMORIA_ADD_TEST_PARAM(step_)->state();

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_sorted_data_file_)->state();

        MEMORIA_ADD_TEST_PARAM(iterator_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(iterator_check_count_)->state();

        MEMORIA_ADD_TEST_PARAM(data_check_counter_)->state();
        MEMORIA_ADD_TEST_PARAM(data_check_count_)->state();
    }

    virtual ~MapTestBase() throw () {
    }

    Key getRandomKey()
    {
        return RNGTool<Key>::next(this);
    }

    Value getRandomValue()
    {
        return RNGTool<Value>::next(this);
    }

    virtual MemBuffer createRandomBuffer(Int size)
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
            Int pairs_size = (Int) pairs.size();

            Int idx = 0;
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

                AssertEQ(MA_SRC, pairs[idx].key_, key);
                AssertEQ(MA_SRC, pairs[idx].value_, value);

                iter.next();
                idx++;
            }

            AssertEQ(MA_SRC, idx, pairs_size);
        }

        data_check_counter_++;
    }

    virtual void checkIterator(Iterator& iter, const char* source)
    {
//         auto cache1 = iter.cache();
// 
//         auto tmp = iter;
//         tmp.refresh();
// 
//         auto cache2 = tmp.cache();
// 
//         if (cache1 != cache2)
//         {
//             iter.dump(out());
//             throw TestException(source, SBuf()<<"Invalid iterator cache. Iterator: "<<cache1<<" Actual: "<<cache2);
//         }
    }

    virtual Key makeRandomKey() = 0;
    virtual Value makeRandomValue() = 0;


    virtual void setUp()
    {
        Base::setUp();

        if (!this->isReplayMode())
        {
            pairs.clear();
            pairs_sorted.clear();

            for (Int c = 0; c < size_; c++)
            {
                pairs.push_back(Pair(makeRandomKey(), makeRandomValue()));
            }
        }
        else {
            LoadVector(pairs, pairs_data_file_);
            LoadVector(pairs_sorted, pairs_sorted_data_file_);
        }
    }

    virtual void onException() noexcept
    {
        Base::onException();

        String basic_name = "Data." + this->getName();

        String pairs_name = basic_name + ".pairs.txt";
        pairs_data_file_ = this->getResourcePath(pairs_name);

        StoreVector(pairs, pairs_data_file_);

        String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
        pairs_sorted_data_file_ = this->getResourcePath(pairs_sorted_name);

        StoreVector(pairs_sorted, pairs_sorted_data_file_);
    }
};

}}