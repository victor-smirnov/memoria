
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>

#include <memoria/v1/containers/labeled_tree/container/ltree_c_api.hpp>
#include <memoria/v1/containers/labeled_tree/container/ltree_c_find.hpp>
#include <memoria/v1/containers/labeled_tree/container/ltree_c_insert.hpp>
#include <memoria/v1/containers/labeled_tree/container/ltree_c_update.hpp>
#include <memoria/v1/containers/labeled_tree/container/ltree_c_remove.hpp>
#include <memoria/v1/containers/labeled_tree/container/ltree_c_checks.hpp>

#include <memoria/v1/containers/labeled_tree/iterator/ltree_i_api.hpp>


#include <memoria/v1/containers/seq_dense/seqd_factory.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_walkers.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_factory.hpp>

namespace memoria {
namespace v1 {



template <typename Profile, typename... LabelDescriptors>
struct BTTypes<Profile, v1::LabeledTree<LabelDescriptors...>>: BTTypes<Profile, v1::BT> {

    typedef BTTypes<Profile, v1::BT>                                            Base;

    static constexpr int32_t BitsPerSymbol  = 1;
    static constexpr int32_t Symbols        = 2;

    using SymbolsSubstreamPath = IntList<0, 1>;

    using StreamDescriptors = FailIf<TL<
            StreamTF<
                TL<
                    StreamSize,
                    TL<PkdFSSeq<typename PkdFSSeqTF<BitsPerSymbol>::Type>>
                >,
                FSEBranchStructTF
            >,
            StreamTF<
                TL<
                    StreamSize,
                    typename louds::StreamDescriptorsListHelper<LabelDescriptors...>::LeafType
                >,
                FSEBranchStructTF,
                TL<
                    TL<>,
                    TL<typename louds::StreamDescriptorsListHelper<LabelDescriptors...>::IdxList>
                >
            >
    >, false>;



    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,

                seq_dense::CtrFindName,
                seq_dense::CtrInsertName,
                seq_dense::CtrRemoveName,

                louds::CtrApiName,
                louds::CtrFindName,
                louds::CtrInsertName,
                louds::CtrUpdateName,
                louds::CtrRemoveName,
                louds::CtrChecksName
    >;

    using FixedLeafContainerPartsList = MergeLists<
                typename Base::FixedLeafContainerPartsList,

                seq_dense::CtrInsertFixedName
    >;

    using VariableLeafContainerPartsList = MergeLists<
                typename Base::VariableLeafContainerPartsList,

                seq_dense::CtrInsertVariableName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,

                seq_dense::IterSelectName,
                seq_dense::IterMiscName,
                seq_dense::IterCountName,
                seq_dense::IterRankName,
                seq_dense::IterSkipName,

                louds::ItrApiName
    >;

    using LabelsTuple = typename louds::LabelsTupleTF<LabelDescriptors...>::Type;

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef v1::louds::LOUDSIteratorCache<Iterator, Container> Type;
    };
};



template <typename Profile, typename... LabelDescriptors, typename T>
class CtrTF<Profile, v1::LabeledTree<LabelDescriptors...>, T>: public CtrTF<Profile, v1::BT, T> {

    using Base = CtrTF<Profile, v1::BT, T>;
public:

    struct Types: Base::Types
    {
        using LeafStreamsStructList = FailIf<typename Base::Types::LeafStreamsStructList, false>;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;

};


}}
