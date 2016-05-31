
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

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_factory.hpp>
#include "../bt/bt_test_base.hpp"
#include "container/btfl_test_factory.hpp"

#include <functional>

namespace memoria {
namespace v1 {

using namespace std;


template <
    typename ContainerTypeName,
    typename AllocatorType,
    typename Profile
>
class BTFLTestBase;

template <
    Int Levels,
    PackedSizeType SizeType,
    typename AllocatorType,
    typename Profile
>
class BTFLTestBase<BTFLTestCtr<Levels, SizeType>, AllocatorType, Profile>: public BTTestBase<BTFLTestCtr<Levels, SizeType>, AllocatorType, Profile> {

    using ContainerTypeName = BTFLTestCtr<Levels, SizeType>;

    using MyType = BTFLTestBase<
                ContainerTypeName,
                Profile,
                AllocatorType
    >;

    using Base = BTTestBase<ContainerTypeName, AllocatorType, Profile>;

protected:
    using Ctr               = typename CtrTF<Profile, ContainerTypeName>::Type;
    using Iterator          = typename Ctr::Iterator;
    using IteratorPtr       = typename Ctr::IteratorPtr;

    using Allocator     = AllocatorType;

    using CtrSizesT     = typename Ctr::Types::CtrSizesT;
    using CtrSizeT      = typename Ctr::Types::CtrSizeT;

    using Key       = typename Ctr::Types::Key;
    using Value     = typename Ctr::Types::Value;
    using Column    = typename Ctr::Types::Column;

    static const Int Streams     	= Ctr::Types::Streams;
    static const Int DataStreams 	= Ctr::Types::DataStreams;

    using DataSizesT    = core::StaticVector<CtrSizeT, DataStreams>;

    template <Int Level>
    using BTFLSampleData = typename Ctr::Types::template IOData<Level>;

    using Base::getRandom;
    using Base::commit;
    using Base::drop;
    using Base::branch;
    using Base::allocator;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::storeAllocator;
    using Base::isReplayMode;
    using Base::getResourcePath;
    using Base::int_generator_;

    bool dump = false;

    BigInt size             = 1000000;
    Int iterations          = 5;
    Int level_limit         = 1000;
    Int last_level_limit    = 100;



public:

    BTFLTestBase(StringRef name):
        Base(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(size);
        MEMORIA_ADD_TEST_PARAM(iterations);
        MEMORIA_ADD_TEST_PARAM(dump);
        MEMORIA_ADD_TEST_PARAM(level_limit);
        MEMORIA_ADD_TEST_PARAM(last_level_limit);
    }

    virtual ~BTFLTestBase() noexcept {}

    virtual void smokeCoverage(Int scale)
    {
        size        = 10000 * scale;
        iterations  = 1;
    }

    virtual void smallCoverage(Int scale)
    {
        size        = 100000 * scale;
        iterations  = 1;
    }

    virtual void normalCoverage(Int scale)
    {
        size        = 10000000 * scale;
        iterations  = 1;
    }

    virtual void largeCoverage(Int scale)
    {
        size        = 100000000 * scale;
        iterations  = 10;
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

    auto createSampleBTFLData(const DataSizesT& shape, bool sort = false)
    {
        return btfl_test::BTFLDataSetBuilder<BTFLSampleData<DataStreams>>::build(shape, int_generator_, sort);
    }

    template <typename BTFLDataT>
    size_t dataLength(const BTFLDataT& data)
    {
    	return v1::btfl::BTFLDataComputeLengthHelper<BTFLDataT>(data).compute();
    }




    template <typename K, typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
    void checkEquality(const Container1<std::tuple<K, V>, Args1...>& first, const Container2<std::tuple<K, V>, Args2...>& second)
    {
    	AssertEQ(MA_SRC, first.size(), second.size());

    	auto i2 = second.begin();
    	for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
    	{
    		AssertEQ(MA_SRC, std::get<0>(*i1), std::get<0>(*i2));

    		checkEquality(std::get<1>(*i1), std::get<1>(*i2));
    	}
    }

    template <typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
    void checkEquality(const Container1<V, Args1...>& first, const Container2<V, Args2...>& second)
    {
    	AssertEQ(MA_SRC, first.size(), second.size());

    	auto i2 = second.begin();
    	for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
    	{
    		AssertEQ(MA_SRC, *i1, *i2);
    	}
    }



//    template <typename K, typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
//    void deepCompare(const Container1<std::tuple<K, V>, Args1...>& first, const Container2<std::tuple<K, V>, Args2...>& second)
//    {
//    	AssertEQ(MA_SRC, first.size(), second.size());
//
//    	auto i2 = second.begin();
//    	for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
//    	{
//    		AssertEQ(MA_SRC, std::get<0>(*i1), std::get<0>(*i2));
//
//    		checkEquality(std::get<1>(*i1), std::get<1>(*i2));
//    	}
//    }
//
//    template <typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
//    void deepCompare(const Container1<V, Args1...>& first, const Container2<V, Args2...>& second)
//    {
//    	AssertEQ(MA_SRC, first.size(), second.size());
//
//    	auto i2 = second.begin();
//    	for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
//    	{
//    		AssertEQ(MA_SRC, *i1, *i2);
//    	}
//    }



    template <typename Ctr>
    auto fillCtrRandomly(Ctr&& ctr, const DataSizesT& shape)
    {
        auto iter = ctr->begin();

        long t0 = getTimeInMillis();

        auto data  = createSampleBTFLData(shape);
        auto totals = iter->insert_iodata(data);

        check("Bulk Insertion", MA_SRC);

        long t1 = getTimeInMillis();

        out() << "Creation time: " << FormatTime(t1 - t0) << " consumed: " << totals << endl;

        return data;
    }



};

}}
