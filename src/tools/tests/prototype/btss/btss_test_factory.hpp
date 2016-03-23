
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

//#include <memoria/v1/prototypes/bt_tl/bttl_factory.hpp>
//#include <memoria/v1/prototypes/bt_tl/tools/bttl_tools_random_gen.hpp>

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

//#include <memoria/v1/containers/table/table_factory.hpp>


#include <functional>

namespace memoria {
namespace v1 {


template <PackedSizeType LeafSizeType, PackedSizeType BranchSizeType>
class BTSSTestCtr {};

template <
    PackedSizeType LeafSizeType,
    PackedSizeType BranchSizeType,
    typename CtrSizeT,
    Int Indexes = 1
> struct BTSSTestStreamTF;

template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::FIXED, PackedSizeType::FIXED, CtrSizeT, Indexes> {
    using Type = StreamTF<
            TL<TL<
                StreamSize,
                PkdFQTreeT<CtrSizeT, Indexes>
            >>,
            FSEBranchStructTF,
            TL<TL<TL<>, TL<SumRange<0, Indexes>>>>
    >;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::VARIABLE, PackedSizeType::FIXED, CtrSizeT, Indexes> {
    using Type = StreamTF<
            TL<TL<
                StreamSize,
                PkdVQTreeT<CtrSizeT, Indexes, UByteI7Codec>
            >>,
            FSEBranchStructTF,
            TL<TL<TL<>, TL<SumRange<0, Indexes>>>>
    >;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::FIXED, PackedSizeType::VARIABLE, CtrSizeT, Indexes> {
    using Type = StreamTF<
            TL<TL<
                StreamSize,
                PkdFQTreeT<CtrSizeT, Indexes>
            >>,
            VLQBranchStructTF,
            TL<TL<TL<>, TL<SumRange<0, Indexes>>>>
    >;
};


template <typename CtrSizeT, Int Indexes>
struct BTSSTestStreamTF<PackedSizeType::VARIABLE, PackedSizeType::VARIABLE, CtrSizeT, Indexes> {
    using Type = StreamTF<
            TL<TL<
                StreamSize,
                PkdVDTreeT<CtrSizeT, Indexes, UByteI7Codec>
            >>,
            VLQBranchStructTF,
            TL<TL<TL<>, TL<SumRange<0, Indexes>>>>
    >;
};


template <
    typename Profile,
    PackedSizeType LeafSizeType,
    PackedSizeType BranchSizeType
>
struct BTSSTestTypesBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;


    using CtrSizeT = BigInt;

    using StreamDescriptors = TL<
            typename BTSSTestStreamTF<LeafSizeType, BranchSizeType, CtrSizeT, 1>::Type
    >;

    using Entry = CtrSizeT;
};







template <
    typename Profile,
    PackedSizeType LeafSizeType,
    PackedSizeType BranchSizeType
>
struct BTTypes<Profile, BTSSTestCtr<LeafSizeType, BranchSizeType>>: public BTSSTestTypesBase<Profile, LeafSizeType, BranchSizeType>
{
};


template <typename Profile, PackedSizeType LeafSizeType, PackedSizeType BranchSizeType, typename T>
class CtrTF<Profile, BTSSTestCtr<LeafSizeType, BranchSizeType>, T>: public CtrTF<Profile, v1::BTSingleStream, T> {
    using Base = CtrTF<Profile, v1::BTSingleStream, T>;
public:
};


template <PackedSizeType LeafSizeType, PackedSizeType BranchSizeType>
struct TypeHash<BTSSTestCtr<LeafSizeType, BranchSizeType>>:   UIntValue<
    HashHelper<3011, (Int)LeafSizeType, (Int)BranchSizeType>::Value
> {};


}}