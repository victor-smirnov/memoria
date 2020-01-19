
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

#include "../bt/bt_test_base.hpp"

#include "container/btfl_test_factory.hpp"

#include "btfl_ctr_impl.hpp"

#include <functional>

namespace memoria {
namespace tests {

template <
    typename ContainerTypeName,
    typename AllocatorType,
    typename Profile
>
class BTFLTestBase;

template <
    int32_t Levels,
    PackedDataTypeSize SizeType,
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

    static const int32_t Streams        = Ctr::Types::Streams;
    static const int32_t DataStreams    = Ctr::Types::DataStreams;

    using DataSizesT    = core::StaticVector<CtrSizeT, DataStreams>;

    template <int32_t Level>
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

    int64_t size             = 1000000;
    int32_t iterations          = 5;
    int32_t level_limit         = 1000;
    int32_t last_level_limit    = 100;



public:

    MMA1_STATE_FILEDS(size, iterations, dump, level_limit, last_level_limit);

    BTFLTestBase(){}

    virtual void smokeCoverage(int32_t scale)
    {
        size        = 3000 * scale;
        iterations  = 1;
    }

    virtual void smallCoverage(int32_t scale)
    {
        size        = 100000 * scale;
        iterations  = 1;
    }

    virtual void normalCoverage(int32_t scale)
    {
        size        = 10000000 * scale;
        iterations  = 1;
    }

    virtual void largeCoverage(int32_t scale)
    {
        size        = 100000000 * scale;
        iterations  = 10;
    }

    DataSizesT sampleTreeShape() {
        return sampleTreeShape(level_limit, last_level_limit, size);
    }

    DataSizesT sampleTreeShape(int32_t level_limit, int32_t last_level_limit, CtrSizeT size)
    {
        CtrSizeT   shape_size = 0;
        DataSizesT largest;

        for (int32_t c = 0; c < 10; c++)
        {
            auto shape = sampleSingleTreeShape(level_limit, last_level_limit, size);
            auto size0 = estimateShapeSize(shape);

            if (size0 > shape_size) {
                largest = shape;
            }
        }

        return largest;
    }

    auto estimateShapeSize(const DataSizesT& shape)
    {
        CtrSizeT size = 1;

        for (int32_t c = 0; c < DataSizesT::Indexes; c++)
        {
            auto item = shape[c];
            size *= item > 0 ? item : 1;
        }

        return size;
    }


    DataSizesT sampleSingleTreeShape(int32_t level_limit, int32_t last_level_limit, CtrSizeT size)
    {
        DataSizesT shape;

        DataSizesT limits(level_limit);
        limits[DataStreams - 1] = last_level_limit;

        while(shape[0] == 0)
        {
            int64_t resource = size;

            for (int32_t c = DataStreams - 1; c > 0; c--)
            {
                int32_t level_size = getRandom(limits[c]) + ((c == DataStreams - 1)? 10 : 1);

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
        return btfl::BTFLDataComputeLengthHelper<BTFLDataT>(data).compute();
    }




    template <typename K, typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
    void checkEquality(const Container1<std::tuple<K, V>, Args1...>& first, const Container2<std::tuple<K, V>, Args2...>& second)
    {
        assert_equals(first.size(), second.size());

        auto i2 = second.begin();
        for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
        {
            assert_equals(std::get<0>(*i1), std::get<0>(*i2));

            checkEquality(std::get<1>(*i1), std::get<1>(*i2));
        }
    }

    template <typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
    void checkEquality(const Container1<V, Args1...>& first, const Container2<V, Args2...>& second)
    {
        assert_equals(first.size(), second.size());

        auto i2 = second.begin();
        for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
        {
            assert_equals(*i1, *i2);
        }
    }



//    template <typename K, typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
//    void deepCompare(const Container1<std::tuple<K, V>, Args1...>& first, const Container2<std::tuple<K, V>, Args2...>& second)
//    {
//      AssertEQ(MA_SRC, first.size(), second.size());
//
//      auto i2 = second.begin();
//      for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
//      {
//          AssertEQ(MA_SRC, std::get<0>(*i1), std::get<0>(*i2));
//
//          checkEquality(std::get<1>(*i1), std::get<1>(*i2));
//      }
//    }
//
//    template <typename V, template <typename...> class Container1, template <typename...> class Container2, typename... Args1, typename... Args2>
//    void deepCompare(const Container1<V, Args1...>& first, const Container2<V, Args2...>& second)
//    {
//      AssertEQ(MA_SRC, first.size(), second.size());
//
//      auto i2 = second.begin();
//      for (auto i1 = first.begin(); i1 != first.end(); i1++, i2++)
//      {
//          AssertEQ(MA_SRC, *i1, *i2);
//      }
//    }



    template <typename Ctr>
    auto fillCtrRandomly(Ctr&& ctr, const DataSizesT& shape)
    {
        auto iter = ctr.begin();

        long t0 = getTimeInMillis();

        auto data  = createSampleBTFLData(shape);
        auto totals = iter.insert_iodata(data);

        check("Bulk Insertion", MA_SRC);

        long t1 = getTimeInMillis();

        out() << "Creation time: " << FormatTime(t1 - t0) << " consumed: " << totals << std::endl;

        return data;
    }



};

}}
