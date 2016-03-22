
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/table/table_names.hpp>

#include <memoria/containers/table/container/table_c_api.hpp>
#include <memoria/containers/table/iterator/table_i_misc.hpp>

#include <memoria/containers/table/table_tools.hpp>
#include <memoria/containers/table/table_iterator.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>


#include <tuple>

namespace memoria {



template <
    typename Profile,
    typename Key_,
    typename Value_,
    PackedSizeType SizeType
>
struct TableBTTypesBase: public BTTypes<Profile, memoria::BTTreeLayout> {

    using Base = BTTypes<Profile, memoria::BTTreeLayout>;

    using ValueType = IfThenElse<
                    IfTypesEqual<Value_, IDType>::Value,
                    typename Base::ID,
                    Value_
    >;


    using Key   = Key_;
    using Value = Value_;

    using CtrSizeT = BigInt;

    static constexpr Int Levels = 3;

    using FirstStreamVariableTF = StreamTF<
        TL<
            //PkdVTree<Packed2TreeTypes<CtrSizeT, CtrSizeT, 1, UByteI7Codec>>
        >,
        FSEBranchStructTF,
        TL<
            //IndexRange<0, 1>
        >

    >;

    using StreamVariableTF = StreamTF<
        TL<>,
        TL<>,
        FSEBranchStructTF
    >;


    using FirstStreamFixedTF = StreamTF<
        TL<
            //PkdFQTree<CtrSizeT, 1>
        >,
        FSEBranchStructTF,
        TL<
            //TL<IndexRange<0, 1>>
        >
    >;

    using StreamFixedTF = StreamTF<
        TL<>,
        FSEBranchStructTF,
        TL<>
    >;

    using DataStreamTF  = StreamTF<
        TL<TL<PkdFSQArrayT<Value, 1>>>,
        FSEBranchStructTF,
        TL<TL<TL<>>>
    >;


    using RawStreamDescriptors = IfThenElse<
            SizeType == PackedSizeType::FIXED,
            MergeLists<
                FirstStreamFixedTF,
                typename MakeList<StreamFixedTF, Levels - 2>::Type,
                DataStreamTF
            >,
            MergeLists<
                FirstStreamVariableTF,
                typename MakeList<StreamVariableTF, Levels - 2>::Type,
                DataStreamTF
            >
    >;

    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
            RawStreamDescriptors,
            //PkdVDTreeT<CtrSizeT, 1, UByteI7Codec>
            PkdFQTreeT<CtrSizeT, 1>
    >::Type;


    using Metadata = BalancedTreeMetadata<
            typename Base::ID,
            ListSize<StreamDescriptors>::Value
    >;


    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::table::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::table::ItrMiscName
    >;
};







template <
    typename Profile,
    typename Key_,
    typename Value_,
    PackedSizeType SizeType
>
struct BTTypes<Profile, memoria::Table<Key_, Value_, SizeType>>: public TableBTTypesBase<Profile, Key_, Value_, SizeType>
{
};


template <typename Profile, typename Key, typename Value, PackedSizeType SizeType, typename T>
class CtrTF<Profile, memoria::Table<Key, Value, SizeType>, T>: public CtrTF<Profile, memoria::BTTreeLayout, T> {
    using Base = CtrTF<Profile, memoria::BTTreeLayout, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = TableCtrTypes<Types>;
        using IterTypes         = TableIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}
