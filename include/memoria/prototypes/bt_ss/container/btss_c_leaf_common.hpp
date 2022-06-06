
// Copyright 2015-2021 Victor Smirnov
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/walkers/bt_misc_walkers.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/prototypes/bt_ss/btss_names.hpp>
#include <memoria/prototypes/bt_ss/btss_input_iovector.hpp>


#include <memoria/core/container/macros.hpp>
#include <memoria/core/iovector/io_vector.hpp>
#include <memoria/api/common/ctr_api_btss.hpp>


#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::LeafCommonName)

    using typename Base::Iterator;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::Profile;
    using typename Base::TreePathT;

    using typename Base::CtrSizeT;
    using typename Base::LeafNode;

    using SplitResultT = bt::SplitResult<CtrSizeT>;

    CtrSharedPtr<BTSSIterator<Profile>> raw_iterator() {
        return self().ctr_begin();
    }

    void dump_leafs(CtrSizeT leafs)
    {
//        auto ii = self().ctr_begin();

//        CtrSizeT lim = leafs >= 0? leafs : std::numeric_limits<CtrSizeT>::max();

//        for (CtrSizeT cc = 0; cc < lim && !ii->is_end(); cc++) {
//        ii->dump();
//            if (!ii->iter_next_leaf()) {
//                break;
//            }
//        }
    }

    template <typename SubstreamsIdxList, typename... Args>
    auto iter_read_leaf_entry(const TreeNodeConstPtr& leaf, Args&&... args) const
    {
         return self().template ctr_apply_substreams_fn<0, SubstreamsIdxList>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    bool ctr_is_at_the_end(const TreeNodeConstPtr& leaf, const Position& pos)
    {
        auto size = self().template ctr_get_leaf_stream_size<0>(leaf);
        return pos[0] >= size;
    }

    template <typename EntryBuffer>
    void iter_insert_entry(Iterator& iter, const EntryBuffer& entry)
    {
        self().template ctr_insert_stream_entry<0>(iter, iter.iter_stream(), iter.iter_local_pos(), entry);
    }




    template <typename SubstreamsList, typename EntryBuffer>
    void ctr_update_entry(Iterator& iter, const EntryBuffer& entry)
    {
        return self().template ctr_update_stream_entry<SubstreamsList>(iter, iter.iter_stream(), iter.iter_local_pos(), entry);
    }


    CtrSizeT ctr_insert_iovector(Iterator& iter, io::IOVectorProducer& producer, CtrSizeT start, CtrSizeT length)
    {
        auto& self = this->self();

        auto iov = LeafNode::template SparseObject<MyType>::create_iovector();
        auto id = iter.iter_leaf()->id();

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, iov.get(), start, length);

        auto pos = Position(iter.iter_local_pos());

        self.ctr_insert_provided_data(iter.path(), pos, streaming);

        iter.iter_local_pos() = pos.sum();

        iter.refresh_iovector_view();

        if (iter.iter_leaf()->id() != id)
        {
            iter.iter_refresh();
        }

        return streaming.totals();
    }

    struct BTSSIOVectorProducer: io::IOVectorProducer {
        virtual bool populate(io::IOVector& io_vector)
        {
            return true; // provided io_vector has been already populated
        }
    };


    CtrSizeT ctr_insert_iovector(Iterator& iter, io::IOVector& io_vector, CtrSizeT start, CtrSizeT length)
    {
        auto& self = this->self();

        //std::unique_ptr<io::IOVector> iov = LeafNode::create_iovector();

        auto id = iter.iter_leaf()->id();

        BTSSIOVectorProducer producer{};

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, &io_vector, start, length, false);

        auto pos = Position(iter.iter_local_pos());

        auto result = self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming);

        iter.iter_local_pos() = result.position().sum();
        iter.iter_leaf().assign(result.iter_leaf());

        if (iter.iter_leaf()->id() != id)
        {
            iter.iter_refresh();
        }

        return streaming.totals();
    }


    SplitResultT split_leaf_in_a_half(TreePathT& path, CtrSizeT target_idx)
    {
        auto& self = this->self();

        auto leaf_sizes = self.ctr_get_leaf_sizes(path.leaf());
        CtrSizeT split_idx = div_2(leaf_sizes[0]);

        self.ctr_split_leaf(path, Position::create(0, split_idx));

        if (target_idx > split_idx)
        {
            self.ctr_expect_next_node(path, 0);
            return SplitResultT(bt::SplitStatus::RIGHT, target_idx - split_idx);
        }
        else {
            return SplitResultT(bt::SplitStatus::LEFT, target_idx);
        }
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
