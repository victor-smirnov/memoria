
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input_iovector.hpp>

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::LeafCommonName)

public:
    using typename Base::Types;
    using typename Base::Iterator;

public:
    using typename Base::NodeBaseG;
    using typename Base::BranchNodeEntry;
    using typename Base::Position;

    using SplitFn = std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using typename Base::CtrSizeT;

    using LeafNodeT = typename Types::LeafNode;

    static const int32_t Streams = Types::Streams;

    CtrSharedPtr<BTSSIterator<typename Types::Profile>> raw_iterator() {
        return self().ctr_begin();
    }

    template <typename SubstreamsIdxList, typename... Args>
    auto iter_read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template ctr_apply_substreams_fn<0, SubstreamsIdxList>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    bool ctr_is_at_the_end(const NodeBaseG& leaf, const Position& pos) noexcept
    {
        int32_t size = self().template ctr_get_leaf_stream_size<0>(leaf);
        return pos[0] >= size;
    }

    template <typename EntryBuffer>
    VoidResult iter_insert_entry(Iterator& iter, const EntryBuffer& entry) noexcept
    {
        auto split_status_res = self().template ctr_insert_stream_entry<0>(iter, iter.iter_stream(), iter.iter_local_pos(), entry);
        MEMORIA_RETURN_IF_ERROR(split_status_res);
        return VoidResult::of();
    }




    template <typename SubstreamsList, typename EntryBuffer>
    VoidResult ctr_update_entry(Iterator& iter, const EntryBuffer& entry) noexcept
    {
        return self().template ctr_update_stream_entry<0, SubstreamsList>(iter, iter.iter_stream(), iter.iter_local_pos(), entry);
    }


    VoidResult ctr_remove_entry(Iterator& iter) noexcept {
        return self().template ctr_remove_stream_entry<0>(iter, iter.iter_stream(), iter.iter_local_pos());
    }





    Result<CtrSizeT> ctr_insert_iovector(Iterator& iter, io::IOVectorProducer& producer, CtrSizeT start, CtrSizeT length) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = LeafNodeT::template SparseObject<MyType>::create_iovector();

        auto id = iter.iter_leaf()->id();

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, iov.get(), start, length);

        auto pos = Position(iter.iter_local_pos());

        auto result = self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming);
        MEMORIA_RETURN_IF_ERROR(result);

        iter.iter_local_pos() = result.get().position().sum();
        iter.iter_leaf().assign(result.get().iter_leaf());

        if (iter.iter_leaf()->id() != id)
        {
            MEMORIA_RETURN_IF_ERROR(iter.iter_refresh());
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

        std::unique_ptr<io::IOVector> iov = Types::LeafNode::create_iovector();

        auto id = iter.iter_leaf()->id();

        BTSSIOVectorProducer producer{};

        btss::io::IOVectorBTSSInputProvider<MyType> streaming(self, &producer, &io_vector, start, length, false);

        auto pos = Position(iter.iter_local_pos());

        auto result = self.ctr_insert_provided_data(iter.iter_leaf(), pos, streaming);
        MEMORIA_RETURN_IF_ERROR(result);

        iter.iter_local_pos() = result.get().position().sum();
        iter.iter_leaf().assign(result.get().iter_leaf());

        if (iter.iter_leaf()->id() != id)
        {
            MEMORIA_RETURN_IF_ERROR(iter.iter_refresh());
        }

        return ResultT::of(streaming.totals());
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

}}
