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

#include <memoria/v1/containers/multimap/mmap_factory.hpp>

#include <memoria/v1/tools/profile_tests.hpp>

#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <tuple>

namespace memoria {
namespace v1 {


template<typename MapName>
class MultiMapTestBase: public BTTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>> {
    using MyType    = MultiMapTestBase<MapName>;
    using Base      = BTTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::IteratorPtr;

    using CtrSizesT = typename Ctr::Types::CtrSizesT;
    using CtrSizeT  = typename Ctr::Types::CtrSizeT;

    using Base::out;

    using Key       = typename Ctr::Types::Key;
    using Value     = typename Ctr::Types::Value;

protected:

    static constexpr Int DataStreams = Ctr::Types::DataStreams;

    using DataSizesT = core::StaticVector<CtrSizeT, DataStreams>;

    BigInt size             = 10000000;
    Int level_limit         = 1000;
    Int last_level_limit    = 100;



    Int iterations = 0;
    Int coverage_   = 0;

public:

    using Base::getRandom;

    using MapData = std::vector<std::pair<Key, vector<Value>>>;



    MultiMapTestBase(StringRef name): Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(size);
        MEMORIA_ADD_TEST_PARAM(level_limit);
        MEMORIA_ADD_TEST_PARAM(last_level_limit);
        MEMORIA_ADD_TEST_PARAM(iterations);
    }

    virtual ~MultiMapTestBase() throw () {}


    virtual void smokeCoverage(Int size) {
        coverage_   = size;
        iterations  = 1;
    }

    virtual void smallCoverage(Int size) {
        coverage_   = size * 10;
        iterations  = 10;
    }

    virtual void normalCoverage(Int size) {
        coverage_   = size * 100;
        iterations  = 100;
    }

    virtual void largeCoverage(Int size) {
        coverage_   = size * 1000;
        iterations  = 1000;
    }

    DataSizesT sampleTreeShape() {
        return sampleTreeShape(level_limit, last_level_limit, size);
    }

    DataSizesT sampleTreeShape(Int level_limit, Int last_level_limit, CtrSizeT size)
    {
        DataSizesT shape;

        DataSizesT limits(level_limit);
        limits[DataStreams - 1] = last_level_limit;

        while(shape[0] == 0)
        {
            BigInt resource = size;

            for (Int c = DataStreams - 1; c > 0; c--)
            {
                Int level_size = getRandom(limits[c]) + ((c == DataStreams - 1)? 10 : 1);

                shape[c] = level_size;

                resource = resource / level_size;
            }

            shape[0] = resource;
        }

        return shape;
    }


    void checkData(Ctr& ctr, const MapData& data)
    {
        AssertEQ(MA_RAW_SRC, ctr.size(), data.size());

        size_t c = 0;
        for (auto iter = ctr.begin(); !iter->is_end(); c++)
        {
            auto key = iter->key();

            auto values_size = iter->count_values();

            if (iter->next())
            {
                auto value = iter->read_values();

                AssertEQ(MA_RAW_SRC, values_size, std::get<1>(data[c]).size());

                AssertEQ(MA_RAW_SRC, key, std::get<0>(data[c]));
                AssertEQ(MA_RAW_SRC, value, std::get<1>(data[c]));
            }
            else {
                break;
            }
        }
    }

    void checkRunPositions(Ctr& ctr)
    {
        size_t c = 0;
        for (auto iter = ctr.begin(); !iter->is_end(); c++)
        {
            auto values_size = iter->count_values();

            if (iter->next())
            {
                auto run_pos = iter->run_pos();
                AssertEQ(MA_RAW_SRC, run_pos, 0);

                if (values_size > 1)
                {
                    auto target_pos = values_size / 2;

                    iter->skipFw(target_pos);

                    auto run_pos = iter->run_pos();
                    AssertEQ(MA_RAW_SRC, run_pos, target_pos);

                    iter->skipFw(values_size - target_pos);
                }
                else {
                    iter->skipFw(values_size);
                }
            }
            else {
                break;
            }
        }
    }


    template <typename Fn1, typename Fn2>
    MapData createMapData(size_t keys, size_t values, Fn1&& key_fn, Fn2&& value_fn)
    {
        MapData data;

        for (size_t c = 0;c < keys; c++)
        {
            vector<Value> val;

            for (size_t v = 0; v < values; v++)
            {
                val.push_back(value_fn(c, v));
            }

            data.push_back(
                make_pair(key_fn(c), std::move(val))
            );
        }

        return data;
    }

    template <typename Fn2>
    vector<Value> createValueData(size_t values, Fn2&& value_fn)
    {
        vector<Value> data;

        for (size_t v = 0; v < values; v++)
        {
            data.push_back(value_fn(v));
        }

        return data;
    }

    template <typename Fn2>
    vector<Key> createKeyData(size_t keys, Fn2&& key_fn)
    {
        vector<Key> data;

        for (size_t v = 0; v < keys; v++)
        {
            data.push_back(key_fn(v));
        }

        return data;
    }



    MapData createRandomShapedMapData(size_t keys, size_t values, std::function<Key(size_t)> key_fn, std::function<Value (size_t, size_t)> value_fn)
    {
        MapData data;

        for (size_t c = 0;c < keys; c++)
        {
            vector<Value> val;

            size_t values_size = this->getRandom(values);

            for (size_t v = 0; v < values_size; v++)
            {
                val.push_back(value_fn(c, v));
            }

            data.push_back(
                make_pair(key_fn(c), std::move(val))
            );
        }

        return data;
    }

    template <typename V, typename T>
    T make_key(V&& num, TypeTag<T>) {
        return num;
    }

    template <typename V>
    String make_key(V&& num, TypeTag<String>)
    {
        stringstream ss;
        ss<<"'";
        ss.width(16);
        ss << num;
        ss<<"'";
        return ss.str();
    }

    template <typename V>
    UUID make_key(V&& num, TypeTag<UUID>)
    {
        return UUID(0, num);
    }



    template <typename V, typename T>
    T make_value(V&& num, TypeTag<T>) {
        return num;
    }

    template <typename V>
    String make_value(V&& num, TypeTag<String>)
    {
        stringstream ss;
        ss << num;
        return ss.str();
    }

    template <typename V>
    UUID make_value(V&& num, TypeTag<UUID>)
    {
        if (num != 0) {
            return UUID::make_random();
        }
        else {
            return UUID();
        }
    }
};

}}
