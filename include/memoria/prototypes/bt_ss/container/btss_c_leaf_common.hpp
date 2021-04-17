
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

    CtrSharedPtr<BTSSIterator<Profile>> raw_iterator() {
        return self().ctr_begin();
    }

    template <typename SubstreamsIdxList, typename... Args>
    auto iter_read_leaf_entry(const TreeNodeConstPtr& leaf, Args&&... args) const noexcept
    {
         return self().template ctr_apply_substreams_fn<0, SubstreamsIdxList>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    BoolResult ctr_is_at_the_end(const TreeNodeConstPtr& leaf, const Position& pos) noexcept
    {
        MEMORIA_TRY(size, self().template ctr_get_leaf_stream_size<0>(leaf));
        return BoolResult::of(pos[0] >= size);
    }

    template <typename EntryBuffer>
    VoidResult iter_insert_entry(Iterator& iter, const EntryBuffer& entry) noexcept
    {
        MEMORIA_TRY_VOID(self().template ctr_insert_stream_entry<0>(iter, iter.iter_stream(), iter.iter_local_pos(), entry));
        return VoidResult::of();
    }




    template <typename SubstreamsList, typename EntryBuffer>
    VoidResult ctr_update_entry(Iterator& iter, const EntryBuffer& entry) noexcept
    {
        return self().template ctr_update_stream_entry<SubstreamsList>(iter, iter.iter_stream(), iter.iter_local_pos(), entry);
    }








    Result<CtrSizeT> ctr_insert_iovector(Iterator& iter, io::IOVectorProducer& producer, CtrSizeT start, CtrSizeT length) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        MEMORIA_TRY(iov, LeafNode::template SparseObject<MyType>::create_iovector());

        auto id = iter.iter_leaf()->id();

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, iov.get(), start, length);

        auto pos = Position(iter.iter_local_pos());

        MEMORIA_TRY_VOID(self.ctr_insert_provided_data(iter.path(), pos, streaming));

        iter.iter_local_pos() = pos.sum();

        iter.refresh_iovector_view();

        if (iter.iter_leaf()->id() != id)
        {
            MEMORIA_TRY_VOID(iter.iter_refresh());
        }

        return ResultT::of(streaming.totals());
    }

    struct BTSSIOVectorProducer: io::IOVectorProducer {
        virtual bool populate(io::IOVector& io_vector)
        {
            return true; // provided io_vector has been already populated
        }
    };


    Result<CtrSizeT> ctr_insert_iovector(Iterator& iter, io::IOVector& io_vector, CtrSizeT start, CtrSizeT length) noexcept
    {
        using ResultT = Result<CtrSizeT>;

        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = LeafNode::create_iovector();

        auto id = iter.iter_leaf()->id();

        BTSSIOVectorProducer producer{};

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, &io_vector, start, length, false);

        auto pos = Position(iter.iter_local_pos());

        MEMORIA_TRY(result, self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming));

        iter.iter_local_pos() = result.position().sum();
        iter.iter_leaf().assign(result.iter_leaf());

        if (iter.iter_leaf()->id() != id)
        {
            MEMORIA_TRY_VOID(iter.iter_refresh());
        }

        return ResultT::of(streaming.totals());
    }


    Result<SplitResult> split_leaf_in_a_half(TreePathT& path, int32_t target_idx) noexcept
    {
        using ResultT = Result<SplitResult>;
        auto& self = this->self();

        MEMORIA_TRY(leaf_sizes, self.ctr_get_leaf_sizes(path.leaf()));
        int32_t split_idx = leaf_sizes[0] / 2;

        MEMORIA_TRY_VOID(self.ctr_split_leaf(path, Position::create(0, split_idx)));

        if (target_idx > split_idx)
        {
            MEMORIA_TRY_VOID(self.ctr_expect_next_node(path, 0));
            return ResultT::of(SplitStatus::RIGHT, target_idx - split_idx);
        }
        else {
            return ResultT::of(SplitStatus::LEFT, target_idx);
        }
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
