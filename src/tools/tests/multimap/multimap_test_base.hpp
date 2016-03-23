// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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


template<typename MapName>
class MultiMapTestBase: public BTTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>> {
    using MyType    = MultiMapTestBase<MapName>;
    using Base      = BTTestBase<MapName, PersistentInMemAllocator<>, DefaultProfile<>>;

public:
    using typename Base::Ctr;
    using typename Base::CtrSizesT;
    using typename Base::IteratorPtr;

    using Base::out;

    using Key       = typename Ctr::Types::Key;
    using Value     = typename Ctr::Types::Value;

protected:

    CtrSizesT sizes_;
    Int iterations_ = 10;


public:

    using MapData = std::vector<std::pair<Key, vector<Value>>>;



    MultiMapTestBase(StringRef name): Base(name)
    {
        Ctr::initMetadata();

        sizes_ = CtrSizesT({10000, 100});

        MEMORIA_ADD_TEST_PARAM(sizes_);
        MEMORIA_ADD_TEST_PARAM(iterations_);
    }

    virtual ~MultiMapTestBase() throw () {}

    void checkData(Ctr& ctr, const MapData& data)
    {
        AssertEQ(MA_RAW_SRC, ctr.size(), data.size());

        size_t c = 0;
        for (auto iter = ctr.begin(); !iter->is_end(); iter->next(), c++)
        {
            auto key    = iter->key();
            auto value  = iter->read_values();

            AssertEQ(MA_RAW_SRC, key, std::get<0>(data[c]));
            AssertEQ(MA_RAW_SRC, value, std::get<1>(data[c]));

            iter->toIndex();
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


    template <typename Fn1, typename Fn2>
    MapData createRandomShapedMapData(size_t keys, size_t values, Fn1&& key_fn, Fn2&& value_fn)
    {
        MapData data;

        for (size_t c = 0;c < keys; c++)
        {
            vector<Value> val;

            size_t values_size = getRandomG(values);

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



    template <typename T> struct TypeTag {};

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

}
