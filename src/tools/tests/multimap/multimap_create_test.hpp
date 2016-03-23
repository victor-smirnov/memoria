// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

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
class MultiMapCreateTest: public MultiMapTestBase<MapName> {

    using MyType = MultiMapCreateTest<MapName>;
    using Base   = MultiMapTestBase<MapName>;


    using typename Base::IteratorPtr;
    using typename Base::Key;
    using typename Base::Value;
    using typename Base::Ctr;

    template <typename T>
    using TypeTag = typename Base::template TypeTag<T>;

    using Base::sizes_;

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

    MultiMapCreateTest(StringRef name): Base(name)
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(runCreateTest, replayCreateTest);
    }

    virtual ~MultiMapCreateTest() throw () {}

    void runCreateTest()
    {
        auto snp = branch();
        auto map = create<MapName>(snp);


        auto map_data = createRandomShapedMapData(
                sizes_[0],
                sizes_[1],
                [this](auto k) {return make_key(k, TypeTag<Key>());},
                [this](auto k, auto v) {return make_value(getRandom(), TypeTag<Value>());}
        );

        using EntryAdaptor = mmap::MMapAdaptor<Ctr>;

        auto iter = map->begin();

        EntryAdaptor stream_adaptor(map_data);
        auto totals = iter->bulk_insert(stream_adaptor);

        auto sizes = map->sizes();
        AssertEQ(MA_RAW_SRC, totals, sizes);

        checkData(*map.get(), map_data);

        snp->commit();
    }

    void replayCreateTest()
    {

    }
};

}}