
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
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/walkers/bt_misc_walkers.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafCommonName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    using typename Base::TreePathT;

    using CtrSizeT = typename Types::CtrSizeT;

    static const int32_t Streams = Types::Streams;

public:
    VoidResult ctr_split_leaf(
            TreePathT& path,
            const Position& split_at
    ) noexcept
    {
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr_split_node(path, 0, [&self, &split_at](NodeBaseG& left, NodeBaseG& right) noexcept -> VoidResult {
            MEMORIA_TRY_VOID(self.ctr_split_leaf_node(left, right, split_at));
            // FIXME: handle OpStatus from res here? Or just remove it from the func defn?
            return VoidResult::of();
        }));

        MEMORIA_TRY_VOID(self.ctr_check_path(path));

        return VoidResult::of();
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, OpStatus);
    Result<OpStatus> ctr_split_leaf_node(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at) noexcept
    {
        OpStatus status = self().leaf_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at);
        return Result<OpStatus>::of(status);
    }

public:








    // TODO: apply noexcept expression

    template <int32_t Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto ctr_apply_substreams_fn(NodeBaseG& leaf, Fn&& fn, Args&&... args)
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto ctr_apply_substreams_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename Fn, typename... Args>
    auto ctr_apply_stream_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::StreamNodeFn<Stream>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto ctr_read_substreams(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template ctr_apply_substreams_fn<Stream, SubstreamsIdxList>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    template <int32_t Stream, typename... Args>
    auto ctr_read_stream(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template ctr_apply_stream_fn<Stream>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }







    struct SumFn {
        template <typename Stream>
        auto stream(const Stream& s, int32_t block, int32_t from, int32_t to)
        {
            return s->sum(block, from, to);
        }

        template <typename Stream>
        auto stream(const Stream& s, int32_t from, int32_t to)
        {
            return s->sums(from, to);
        }
    };



    struct FindFn {
        template <typename Stream, typename... Args>
        auto stream(const Stream& s, Args&&... args)
        {
            return s->findForward(std::forward<Args>(args)...).local_pos();
        }
    };

    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto ctr_find_forward(const NodeBaseG& leaf, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), FindFn(), std::forward<Args>(args)...);
    }


    // TODO: error handling
    std::shared_ptr<io::IOVector> create_iovector() noexcept
    {
        return std::static_pointer_cast<io::IOVector>(
            std::make_shared<typename Types::LeafNode::template SparseObject<MyType>::IOVectorT>()
        );
    }


    // ============================================ Insert Data ======================================== //





    VoidResult complete_tree_path(TreePathT& path, const NodeBaseG& node) const noexcept
    {
        auto& self = this->self();

        size_t level = node->level();
        path[level] = node;

        if (!node->is_leaf())
        {
            MEMORIA_TRY(last_child, self.ctr_get_node_last_child(node));
            return complete_tree_path(path, last_child);
        }

        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    Result<SplitStatus> ctr_update_atomic(Iterator& iter, Fn&& fn, Args&&... args) noexcept
    {
        using ResultT = Result<SplitStatus>;

        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf());


        self().leaf_dispatcher().dispatch(
            iter.iter_leaf(),
            fn,
            std::forward<Args>(args)...
        );

        if (isOk(fn.status_)) {
            return SplitStatus::NONE;
        }

        mgr.rollback();

        SplitStatus status = iter.split();

        fn.status_ = OpStatus::OK;

        self().leaf_dispatcher().dispatch(
                    iter.iter_leaf(),
                    fn,
                    std::forward<Args>(args)...
        );

        if (isFail(fn.status_)) {
            return MEMORIA_MAKE_GENERIC_ERROR("PackedOOMException");
        }

        return ResultT::of(status);
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
