
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MULTIMAP_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_FACTORY_HPP

#include <memoria/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/containers/multimap/mmap_names.hpp>

#include <memoria/containers/multimap/container/mmap_c_api.hpp>
#include <memoria/containers/multimap/iterator/mmap_i_misc.hpp>

#include <memoria/containers/multimap/mmap_tools.hpp>
#include <memoria/containers/multimap/mmap_iterator.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

#include <tuple>

namespace memoria {


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBaseBase: public BTTypes<Profile, memoria::BTTreeLayout> {

    using Base = BTTypes<Profile, memoria::BTTreeLayout>;


    using Key   = Key_;
    using Value = Value_;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::mmap::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::mmap::ItrMiscName
    >;
};



template <
    typename Profile,
    typename Key,
    typename Value
>
struct MultimapBTTypesBase: public MultimapBTTypesBaseBase<Profile, Key, Value> {

    using Base = MultimapBTTypesBaseBase<Profile, Key, Value>;

    using CtrSizeT = typename Base::CtrSizeT;

    using FirstStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename mmap::MMapKeyStructTF<Key>::Type>
        >,
        DefaultBranchStructTF
    >;

    using DataStreamTF = StreamTF<
        TL<
            TL<StreamSize>,
            TL<typename mmap::MMapValueStructTF<Value>::Type>
        >,
        DefaultBranchStructTF
    >;


    using RawStreamDescriptors = TL<
            FirstStreamTF,
            DataStreamTF
    >;

    using StreamDescriptors = typename bttl::BTTLAugmentStreamDescriptors<
            RawStreamDescriptors
    >::Type;
};







template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::Map<Key_, Vector<Value_>>>: public MultimapBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Vector<Value>>, T>: public CtrTF<Profile, memoria::BTTreeLayout, T> {
    using Base = CtrTF<Profile, memoria::BTTreeLayout, T>;
public:

    struct Types: Base::Types
    {
        using CtrTypes          = MultimapCtrTypes<Types>;
        using IterTypes         = MultimapIterTypes<Types>;

        using PageUpdateMgr     = PageUpdateManager<CtrTypes>;

        using LeafStreamsStructList = FailIf<typename Base::Types::LeafStreamsStructList, false>;

        using IteratorBranchNodeEntry = FailIf<typename Base::Types::IteratorBranchNodeEntry, false>;
    };

    using CtrTypes  = typename Types::CtrTypes;
    using Type      = Ctr<CtrTypes>;
};


}

#endif
