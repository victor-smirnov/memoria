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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include "multimap_test_base.hpp"

namespace memoria {
namespace v1 {

template <
    typename MapName
>
class MultiMapRemoveTest: public MultiMapTestBase<MapName> {

    using MyType = MultiMapRemoveTest<MapName>;
    using Base   = MultiMapTestBase<MapName>;

    using typename Base::Ctr;
    using typename Base::Allocator;
    using typename Base::IteratorPtr;
    using typename Base::Key;
    using typename Base::Value;
    using typename Base::MapData;

    template <typename T>
    using TypeTag = typename Base::template TypeTag<T>;

    using Base::sizes_;
    using Base::iterations_;

    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::getRandom;

    using Base::checkData;
    using Base::out;

    using Base::createRandomShapedMapData;
    using Base::make_key;
    using Base::make_value;



public:

    MultiMapRemoveTest(StringRef name): Base(name)
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(runRemoveKeysTest, replayRemoveKeysTest);
        MEMORIA_ADD_TEST_WITH_REPLAY(runRemoveValuesTest, replayRemoveValuesTest);
    }

    virtual ~MultiMapRemoveTest() throw () {}

    void removeKeys(MapData& data, size_t at, size_t size = 1)
    {
        if (at + size > data.size())
        {
            size = data.size() - at;
        }

        data.erase(data.begin() + at, data.begin() + at + size);
    }

    void removeValues(MapData& data, size_t key_num, size_t at, size_t size = 1)
    {
        AssertLT(MA_RAW_SRC, key_num, data.size());

        auto& values = std::get<1>(data[key_num]);

        if (at + size > values.size())
        {
            size = values.size() - at;
        }

        values.erase(values.begin() + at, values.begin() + at + size);
    }

    void runRemoveKeysTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        auto map_data = createRandomShapedMapData(
                sizes_[0],
                sizes_[1],
                [this](auto k) {return this->make_key(k, TypeTag<Key>());},
                [this](auto k, auto v) {return this->make_value(this->getRandom(), TypeTag<Value>());}
        );

        using EntryAdaptor = mmap::MMapAdaptor<Ctr>;

        auto iter = map->begin();

        EntryAdaptor stream_adaptor(map_data);
        auto totals = iter->bulk_insert(stream_adaptor);

        auto sizes = map->sizes();
        AssertEQ(MA_RAW_SRC, totals, sizes);

        for (Int c = 0; c < iterations_; c++)
        {
            size_t idx = getRandom(map_data.size());

            removeKeys(map_data, idx);

            map->seek(idx)->remove();

            checkData(*map.get(), map_data);
        }

        snp->commit();
    }

    void replayRemoveKeysTest()
    {

    }

    void runRemoveValuesTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);

        auto map_data = createRandomShapedMapData(
                sizes_[0],
                sizes_[1],
                [this](auto k) {return this->make_key(k, TypeTag<Key>());},
                [this](auto k, auto v) {return this->make_value(this->getRandom(), TypeTag<Value>());}
        );

        using EntryAdaptor = mmap::MMapAdaptor<Ctr>;

        auto iter = map->begin();

        EntryAdaptor stream_adaptor(map_data);
        auto totals = iter->bulk_insert(stream_adaptor);

        auto sizes = map->sizes();
        AssertEQ(MA_RAW_SRC, totals, sizes);

        for (Int c = 0; c < iterations_; c++)
        {
            size_t key_idx = getRandom(map_data.size());
            size_t value_idx = getRandom(std::get<1>(map_data[key_idx]).size());
            size_t value_len = getRandom(std::get<1>(map_data[key_idx]).size() - value_idx);

            removeValues(map_data, key_idx, value_idx, value_len);

            auto iter2 = map->seek(key_idx);
            iter2->toData(value_idx);
            iter2->remove(value_len);

            checkData(*map.get(), map_data);
        }

        snp->commit();
    }

    void replayRemoveValuesTest()
    {

    }

};

}}
