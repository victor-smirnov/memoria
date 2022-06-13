
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
#include <memoria/core/container/macros.hpp>
#include <memoria/core/iovector/io_vector.hpp>
#include <memoria/prototypes/bt/nodes/leaf_node_so.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafWCommonName)

    using typename Base::Types;
    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::TreePathT;
    using typename Base::CtrSizeT;

    using typename Base::BlockIteratorStatePtr;

public:

    void ctr_split_leaf(
            TreePathT& path,
            const Position& split_at
    )
    {
        auto& self = this->self();

        self.ctr_split_node(path, 0, [&self, &split_at](const TreeNodePtr& left, const TreeNodePtr& right) {
            return self.ctr_split_leaf_node(left, right, split_at);
        });
    }

    MEMORIA_V1_DECLARE_NODE_FN(SplitNodeFn, split_to);
    void ctr_split_leaf_node(const TreeNodePtr& src, const TreeNodePtr& tgt, const Position& split_at)
    {
        return self().leaf_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at);
    }

public:




//    struct SumFn {
//        template <typename Stream>
//        auto stream(const Stream& s, size_t block, size_t from, size_t to)
//        {
//            return s->sum(block, from, to);
//        }

//        template <typename Stream>
//        auto stream(const Stream& s, size_t from, size_t to)
//        {
//            return s->sums(from, to);
//        }
//    };




//    IterSharedPtr<io::IOVector> create_iovector()
//    {
//        using IOVectorT = typename Types::LeafNode::template SparseObject<MyType>::IOVectorT;
//        return allocate_shared<IOVectorT>(self().store().object_pools());
//    }


    // ============================================ Insert Data ======================================== //





    void complete_tree_path(TreePathT& path, const TreeNodeConstPtr& node) const
    {
        auto& self = this->self();

        size_t level = node->level();
        path[level] = node;

        if (!node->is_leaf())
        {
            auto last_child = self.ctr_get_node_last_child(node);
            return complete_tree_path(path, last_child);
        }
    }


    template <size_t Stream>
    struct PrepareInsertFn {
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <
            size_t Idx,
            typename SubstreamType,
            typename Entry,
            typename UpdateState
        >
        void stream(SubstreamType&& obj, size_t idx, const Entry& entry, UpdateState& update_state) {
            if (is_success(status)) {
                status = obj.prepare_insert(idx, 1, std::get<Idx>(update_state), [&](psize_t block, psize_t row) -> const auto& {
                    return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
                });
            }
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(const LeafNodeSO<CtrT, NTypes>& node, size_t idx, Args&&... args) {
            return node.template processSubstreams<IntList<Stream>>(*this, idx, std::forward<Args>(args)...);
        }
    };


    template <size_t Stream>
    struct CommitInsertFn {
        template <
            size_t Idx,
            typename SubstreamType,
            typename Entry,
            typename UpdateState
        >
        void stream(SubstreamType&& obj, size_t idx, const Entry& entry, UpdateState& update_state)
        {
            return obj.commit_insert(idx, 1, std::get<Idx>(update_state), [&](psize_t block, psize_t row) -> const auto& {
                return entry.get(bt::StreamTag<Stream>(), bt::StreamTag<Idx>(), block);
            });
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, size_t idx, Args&&... args) {
            return node.template processSubstreams<IntList<Stream>>(*this, idx, std::forward<Args>(args)...);
        }
    };



    template <size_t Stream, typename Entry>
    bool ctr_try_insert_stream_entry_no_mgr(const TreeNodePtr& leaf, size_t idx, const Entry& entry)
    {
        auto update_state = self().template ctr_make_leaf_update_state<IntList<Stream>>(leaf.as_immutable());

        PrepareInsertFn<Stream> fn1;
        self().leaf_dispatcher().dispatch(leaf, fn1, idx, entry, update_state);
        if (is_success(fn1.status)) {
            CommitInsertFn<Stream> fn2;
            self().leaf_dispatcher().dispatch(leaf, fn2, idx, entry, update_state);
            return true;
        }
        else {
            return false;
        }
    }



    template <size_t Stream, typename Entry>
    bool ctr_try_insert_stream_entry(
            TreePathT& path,
            CtrSizeT idx,
            const Entry& entry
    )
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, 0);
        self.ctr_update_block_guard(path.leaf());

        return self.template ctr_try_insert_stream_entry_no_mgr<Stream>(path.leaf().as_mutable(), idx, entry);
    }




    MEMORIA_V1_DECLARE_NODE_FN(TryRemoveFn, remove);
    template <size_t Stream>
    PkdUpdateStatus ctr_try_remove_stream_entry(TreePathT& path, CtrSizeT idx)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, 0);
        self.ctr_update_block_guard(path.leaf());

        TryRemoveFn fn;
        return self.leaf_dispatcher().dispatch(
                path.leaf().as_mutable(),
                fn,
                SizeTList<Stream>{},
                idx, idx + 1
        );
    }

    //=========================================================================================

    template <typename SubstreamsList>
    struct CommitUpdateStreamFn
    {
        template <
            size_t StreamIdx,
            size_t AllocatorIdx,
            size_t Idx,
            typename SubstreamType,
            typename Entry,
            typename UpdateState
        >
        void stream(SubstreamType&& obj, size_t idx, const Entry& entry, UpdateState& update_state)
        {
            return obj.commit_update(idx, 1, std::get<Idx>(update_state), [&](auto block, auto){
                return entry.get(bt::StreamTag<StreamIdx>(), bt::StreamTag<Idx>(), block);
            });
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, size_t idx, Args&&... args)
        {
            return node.template processSubstreams<
                SubstreamsList
            >(
                    *this,
                    idx,
                    std::forward<Args>(args)...
            );
        }
    };

    template <typename SubstreamsList>
    struct PrepareUpdateStreamFn
    {
        PkdUpdateStatus status {PkdUpdateStatus::SUCCESS};

        template <
            size_t StreamIdx,
            size_t AllocatorIdx,
            size_t Idx,
            typename SubstreamType,
            typename Entry,
            typename UpdateState
        >
        void stream(SubstreamType&& obj, size_t idx, const Entry& entry, UpdateState& update_state)
        {
            if (is_success(status)) {
                status = obj.prepare_update(idx, 1, std::get<Idx>(update_state), [&](auto block, auto){
                    return entry.get(bt::StreamTag<StreamIdx>(), bt::StreamTag<Idx>(), block);
                });
            }
        }

        template <typename CtrT, typename NTypes, typename... Args>
        void treeNode(LeafNodeSO<CtrT, NTypes>& node, size_t idx, Args&&... args)
        {
            return node.template processSubstreams<
                SubstreamsList
            >(
                    *this,
                    idx,
                    std::forward<Args>(args)...
            );
        }
    };

    template <typename SubstreamsList, typename Entry>
    bool ctr_try_update_stream_entry(TreePathT& path, CtrSizeT idx, const Entry& entry)
    {
        auto& self = this->self();

        self.ctr_cow_clone_path(path, 0);
        self.ctr_update_block_guard(path.leaf().as_mutable());

        auto update_state = self.template ctr_make_leaf_update_state<SubstreamsList>(path.leaf());
        PrepareUpdateStreamFn<SubstreamsList> fn1;

        if (Types::LeafDataLength == LeafDataLengthType::VARIABLE)
        {
            self.leaf_dispatcher().dispatch(
                        path.leaf().as_mutable(),
                        fn1,
                        idx,
                        entry,
                        update_state
            );
        }

        if (is_success(fn1.status)) {
            CommitUpdateStreamFn<SubstreamsList> fn2;
            self.leaf_dispatcher().dispatch(
                        path.leaf().as_mutable(),
                        fn2,
                        idx,
                        entry,
                        update_state
            );

            return true;
        }
        return false;
    }



    MEMORIA_V1_DECLARE_NODE_FN(TryMergeNodesFn, merge_with);
    bool ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path);

    bool ctr_merge_leaf_nodes(TreePathT& tgt, TreePathT& src, bool only_if_same_parent = false);

    bool ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src);

    MEMORIA_V1_DECLARE_NODE_FN(RemoveSpaceFn, remove);
    PkdUpdateStatus ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end);

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafWCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::ctr_try_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path)
{
    auto& self = this->self();

    self.ctr_check_same_paths(tgt_path, src_path, 1);

    auto src_parent = self.ctr_get_node_parent(src_path, 0);
    auto parent_idx = self.ctr_get_child_idx(src_parent, src_path.leaf()->id());

    self.ctr_cow_clone_path(tgt_path, 0);

    TreeNodeConstPtr tgt = tgt_path.leaf();
    TreeNodeConstPtr src = src_path.leaf();

    self.ctr_update_block_guard(tgt);

    PkdUpdateStatus res = self.leaf_dispatcher().dispatch_1st_const(src, tgt.as_mutable(), TryMergeNodesFn());

    if (is_success(res))
    {
        self.ctr_remove_branch_node_entry(tgt_path, 1, parent_idx);

        self.ctr_update_path(tgt_path, 0);

        self.ctr_cow_ref_children_after_merge(src.as_mutable());

        self.ctr_unref_block(src->id());

        self.ctr_check_path(tgt_path, 0);

        return true;
    }
    else {
        return false;
    }
}


M_PARAMS
bool M_TYPE::ctr_merge_leaf_nodes(TreePathT& tgt_path, TreePathT& src_path, bool only_if_same_parent)
{
    auto& self = this->self();

    auto same_parent = self.ctr_is_the_same_parent(tgt_path, src_path, 0);

    if (same_parent)
    {
        auto merged = self.ctr_merge_current_leaf_nodes(tgt_path, src_path);
        if (!merged)
        {
            self.ctr_assign_path_nodes(tgt_path, src_path);
            self.ctr_expect_next_node(src_path, 0);
        }

        return merged;
    }
    else if (!only_if_same_parent)
    {
        auto merged = self.ctr_merge_branch_nodes(tgt_path, src_path, 1, only_if_same_parent);

        self.ctr_assign_path_nodes(tgt_path, src_path, 0);
        self.ctr_expect_next_node(src_path, 0);

        if (merged)
        {
            return self.ctr_merge_current_leaf_nodes(tgt_path, src_path);
        }
    }

    return false;
}


M_PARAMS
bool M_TYPE::ctr_merge_current_leaf_nodes(TreePathT& tgt, TreePathT& src)
{
    auto& self = this->self();

    auto merged = self.ctr_try_merge_leaf_nodes(tgt, src);
    if (merged)
    {
        self.ctr_remove_redundant_root(tgt, 0);
        return true;
    }
    else {
        return false;
    }
}


M_PARAMS
PkdUpdateStatus M_TYPE::ctr_remove_leaf_content(TreePathT& path, const Position& start, const Position& end)
{
    auto& self = this->self();

    TreeNodeConstPtr node = path.leaf();
    self.ctr_update_block_guard(node);

    PkdUpdateStatus status = self.leaf_dispatcher().dispatch(node.as_mutable(), RemoveSpaceFn(), start, end);
    if (is_success(status)) {
        self.ctr_update_path(path, 0);
    }

    return status;
}



#undef M_TYPE
#undef M_PARAMS

}
