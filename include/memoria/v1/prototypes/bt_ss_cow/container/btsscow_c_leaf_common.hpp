
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss_cow::LeafCommonName)

public:
    using typename Base::Types;
    using typename Base::Iterator;


public:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;


    using typename Base::BranchNodeEntry;
    using typename Base::Position;

    using SplitFn = std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using typename Base::CtrSizeT;

    static const int32_t Streams = Types::Streams;

    template <typename SubstreamsIdxList, typename... Args>
    auto read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template apply_substreams_fn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    bool isAtTheEnd2(const NodeBaseG& leaf, const Position& pos)
    {
        int32_t size = self().template getLeafStreamSize<0>(leaf);
        return pos[0] >= size;
    }

    template <typename EntryBuffer>
    void insert_entry(Iterator& iter, const EntryBuffer& entry)
    {
        self().template insert_stream_entry<0>(iter, iter.stream(), iter.local_pos(), entry);
    }




    template <typename SubstreamsList, typename EntryBuffer>
    void update_entry(Iterator& iter, const EntryBuffer& entry)
    {
        self().template update_stream_entry<0, SubstreamsList>(iter, iter.stream(), iter.local_pos(), entry);
    }


    void removeEntry(Iterator& iter) {
        self().template remove_stream_entry<0>(iter, iter.stream(), iter.local_pos());
    }



    template <typename InputProvider>
    CtrSizeT insert(Iterator& iter, InputProvider& provider)
    {
        auto& self = this->self();

        auto pos = Position(iter.local_pos());

        auto id = iter.leaf()->id();

        auto result = self.insert_provided_data(iter.leaf(), pos, provider);

        iter.leaf() = result.leaf();
        iter.local_pos() = result.position()[0];

        if (id != iter.leaf()->id())
        {
            iter.refresh();
        }

        return provider.total();
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btss_cow::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

}}
