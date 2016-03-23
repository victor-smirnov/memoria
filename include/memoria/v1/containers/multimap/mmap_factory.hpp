
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt_tl/bttl_factory.hpp>
#include <memoria/v1/containers/multimap/mmap_names.hpp>

#include <memoria/v1/containers/multimap/container/mmap_c_api.hpp>
#include <memoria/v1/containers/multimap/iterator/mmap_i_misc.hpp>

#include <memoria/v1/containers/multimap/mmap_tools.hpp>
#include <memoria/v1/containers/multimap/mmap_iterator.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <tuple>

namespace memoria {
namespace v1 {


template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct MultimapBTTypesBaseBase: public BTTypes<Profile, v1::BTTreeLayout> {

    using Base = BTTypes<Profile, v1::BTTreeLayout>;


    using Key   = Key_;
    using Value = Value_;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                v1::mmap::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                v1::mmap::ItrMiscName
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
struct BTTypes<Profile, v1::Map<Key_, Vector<Value_>>>: public MultimapBTTypesBase<Profile, Key_, Value_>
{
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, v1::Map<Key, Vector<Value>>, T>: public CtrTF<Profile, v1::BTTreeLayout, T> {
    using Base = CtrTF<Profile, v1::BTTreeLayout, T>;
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


}}