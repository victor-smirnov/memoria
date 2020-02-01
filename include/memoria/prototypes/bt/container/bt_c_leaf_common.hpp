
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



    // ============================================ Insert Data ======================================== //


    class LeafList {
        CtrSizeT size_;
        NodeBaseG head_;
        NodeBaseG tail_;
    public:
        LeafList(CtrSizeT size, NodeBaseG head, NodeBaseG tail): size_(size), head_(head), tail_(tail) {}

        CtrSizeT size() const {return size_;}
        const NodeBaseG& head() const {return head_;}
        const NodeBaseG& tail() const {return tail_;}

        NodeBaseG& head() {return head_;}
        NodeBaseG& tail() {return tail_;}
    };



    class InsertDataBlockResult {
        Position inserted_size_;
        bool extra_space_;
    public:
        InsertDataBlockResult(Position size, bool extra_space): inserted_size_(size), extra_space_(extra_space){}

        const Position& inserted_size() const {return inserted_size_;}
        bool has_extra_space() const {return extra_space_;}
    };


    template <typename Provider>
    Result<Position> ctr_insert_data_into_leaf(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
        using ResultT = Result<Position>;
        auto& self = this->self();

        MEMORIA_TRY_VOID(self.ctr_update_block_guard(leaf));
        MEMORIA_TRY_VOID(self.ctr_layout_leaf_node(leaf, Position(0)));

        BoolResult has_data_res = provider.hasData();
        if (has_data_res.get())
        {
            auto end = provider.fill(leaf, pos);
            MEMORIA_RETURN_IF_ERROR(end);

            return ResultT::of(end.get());
        }

        return ResultT::of(pos);
    }


    // TODO: error handling
    std::shared_ptr<io::IOVector> create_iovector() noexcept
    {
        return std::static_pointer_cast<io::IOVector>(
            std::make_shared<typename Types::LeafNode::template SparseObject<MyType>::IOVectorT>()
        );
    }


    class InsertDataResult {
        Position position_;
    public:
        InsertDataResult(const Position& position = Position()): position_(position){}

        const Position& position() const {return position_;}
        Position& position() {return position_;}
    };

    template <typename Provider>
    VoidResult ctr_insert_provided_data(TreePathT& path, Position& pos, Provider& provider) noexcept
    {
        using ResultT = VoidResult;

        auto& self = this->self();

        NodeBaseG leaf = path.leaf();

        MEMORIA_TRY(last_pos, self.ctr_insert_data_into_leaf(leaf, pos, provider));
        MEMORIA_TRY(has_data, provider.hasData());

        if (has_data)
        {
            // has to be defined in subclasses
            if (!self.ctr_is_at_the_end(leaf, last_pos))
            {
                auto split_leaf_res = self.ctr_split_leaf(path, last_pos);
                MEMORIA_RETURN_IF_ERROR(split_leaf_res);

                MEMORIA_TRY(last_leaf_pos, self.ctr_insert_data_into_leaf(leaf, last_pos, provider));
                MEMORIA_TRY(has_data2, provider.hasData());

                if (has_data2)
                {
                    return ctr_insert_data_rest(path, pos, provider);
                }
                else {
                    pos = last_leaf_pos;
                }
            }
            else {
                return ctr_insert_data_rest(path, pos, provider);
            }
        }
        else {
            pos = last_pos;
        }

        return ResultT::of();
    }



    template <typename Provider>
    Result<LeafList> ctr_create_leaf_data_list(Provider& provider) noexcept
    {
        using ResultT = Result<LeafList>;
        auto& self = this->self();

        CtrSizeT    total = 0;
        NodeBaseG   head;
        NodeBaseG   current;

        auto meta_res = self.ctr_get_root_metadata();
        MEMORIA_RETURN_IF_ERROR(meta_res);

        int32_t block_size = meta_res.get().memory_block_size();

        while (true)
        {
            auto has_data_res = provider.hasData();
            MEMORIA_RETURN_IF_ERROR(has_data_res);

            if (!has_data_res.get()) {
                break;
            }

            Result<NodeBaseG> node_res = self.ctr_create_node(0, false, true, block_size);
            MEMORIA_RETURN_IF_ERROR(node_res);

            NodeBaseG node = node_res.get();

            if (head.isSet())
            {
                current->next_leaf_id() = node->id();
            }
            else {
                head = node;
            }

            auto ins_res = self.ctr_insert_data_into_leaf(node, Position(), provider);
            MEMORIA_RETURN_IF_ERROR(ins_res);

            auto next_res = provider.iter_next_leaf(node);
            MEMORIA_RETURN_IF_ERROR(next_res);

            current = node;

            total++;
        }

        total += provider.orphan_splits();

        return ResultT::of(total, head, current);
    }




    template <typename Provider>
    VoidResult ctr_insert_data_rest(TreePathT& path, Position& end_pos, Provider& provider) noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        NodeBaseG leaf = path.leaf();

        auto leaf_list = self.ctr_create_leaf_data_list(provider);
        MEMORIA_RETURN_IF_ERROR(leaf_list);

        if (leaf_list.get().size() > 0)
        {
            if (leaf->is_root())
            {
                MEMORIA_TRY_VOID(self.ctr_create_new_root_block(path));
            }

            MEMORIA_TRY(leaf_parent_idx, self.ctr_get_parent_idx(path, 0));
            int32_t path_parent_idx = leaf_parent_idx + 1;

            using LeafListProvider = typename Base::ListLeafProvider;

            LeafListProvider list_provider(self, leaf_list.get().head(), leaf_list.get().size());

            MEMORIA_TRY_VOID(self.ctr_insert_subtree(path, 1, path_parent_idx, list_provider));

            auto& last_leaf = leaf_list.get().tail();

            auto last_leaf_size = self.ctr_get_leaf_stream_sizes(last_leaf);

            TreePathT next_path = path;
            MEMORIA_TRY(has_next_leaf, self.ctr_get_next_node(next_path, 0));

            if (has_next_leaf)
            {
                MEMORIA_TRY(
                    has_merge,
                    self.ctr_merge_leaf_nodes(path, next_path, [](const Position&){return VoidResult::of();})
                );

                if (has_merge)
                {
                    end_pos = last_leaf_size;
                }
                else {
                    end_pos = Position{};
                    path = next_path;
                }
            }
            else {
                end_pos = last_leaf_size;
            }
        }

        return ResultT::of();
    }


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
            return ResultT::make_error("PackedOOMException");
        }

        return ResultT::of(status);
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
