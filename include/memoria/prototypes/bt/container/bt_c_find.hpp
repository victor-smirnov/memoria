
// Copyright 2011 Victor Smirnov
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

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>

#include <memoria/core/container/macros.hpp>

#include <limits>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::FindName)
public:
    using Types = TypesType;

    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::IteratorPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;

    using LeafStreamsStructList = typename Types::LeafStreamsStructList;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    MEMORIA_V1_DECLARE_NODE_FN(SizesFn, size_sums);
    Position sizes() const
    {
        auto node = self().ctr_get_root_node();
        return self().node_dispatcher().dispatch(node, SizesFn()).get_or_throw();
    }

public:

    template <typename Walker>
    IteratorPtr ctr_find(Walker&& walker) const;

    template <typename LeafPath>
    IteratorPtr ctr_find_gt(int32_t index, const TargetType<LeafPath>& key) const
    {
        typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);
        return self().ctr_find(walker);
    }

    template <typename LeafPath>
    IteratorPtr ctr_find_max_gt(int32_t index, const TargetType<LeafPath>& key) const
    {
        typename Types::template FindMaxGTWalker<Types, LeafPath> walker(index, key);
        return self().ctr_find(walker);
    }


    template <typename LeafPath>
    IteratorPtr ctr_find_ge(int32_t index, const TargetType<LeafPath>& key) const
    {
        typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);
        return self().ctr_find(walker);
    }

    template <typename LeafPath>
    IteratorPtr ctr_find_max_ge(int32_t index, const TargetType<LeafPath>& key) const
    {
        typename Types::template FindMaxGEWalker<Types, LeafPath> walker(index, key);
        return self().ctr_find(walker);
    }

    template <typename LeafPath>
    IteratorPtr ctr_rank(int32_t index, CtrSizeT pos, SeqOpType op_type) const
    {
        typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos, op_type);
        return self().ctr_find(walker);
    }

    template <typename LeafPath>
    IteratorPtr ctr_select(int32_t index, CtrSizeT rank, SeqOpType op_type) const
    {
        typename Types::template SelectForwardWalker<Types, LeafPath> walker(index, rank, op_type);
        return self().ctr_find(walker);
    }


    struct NodeChain {
        const MyType& ctr_;
        TreeNodeConstPtr node;
        int32_t start;
        int32_t end;
        NodeChain* ref;

        NodeChain(const MyType& ctr, const TreeNodeConstPtr& node_, int32_t start_, NodeChain* ref_ = nullptr) noexcept :
            ctr_(ctr),
            node(node_), start(start_), end(0), ref(ref_)
        {}

        void swapRanges()
        {
            auto tmp = start;
            end = start;
            start = tmp;
        }

        template <typename Walker>
        WalkCmd processChain(Walker&& walker, int32_t leaf_cnt = 0)
        {
            if (node->is_leaf())
            {
                leaf_cnt++;
            }

            if (ref)
            {
                ref->processChain(std::forward<Walker>(walker), leaf_cnt);
            }

            if (node->is_leaf())
            {
                WalkCmd cmd = WalkCmd::NONE;

                if (leaf_cnt == 1)
                {
                    if (ref == nullptr) {
                        cmd = WalkCmd::THE_ONLY_LEAF;
                    }
                    else {
                        cmd = WalkCmd::LAST_LEAF;
                    }
                }
                else if (leaf_cnt == 2) {
                    cmd = WalkCmd::FIRST_LEAF;
                }

                ctr_.leaf_dispatcher().dispatch(node, std::forward<Walker>(walker), cmd, start, end).get_or_throw();

                return cmd;
            }
            else {
                ctr_.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end).get_or_throw();

                return WalkCmd::PREFIXES;
            }
        }
    };

    struct FindResult {
        int32_t     idx;
        bool        pass;
        WalkCmd     cmd;

        explicit FindResult(int32_t idx_, WalkCmd cmd_, bool pass_ = true) noexcept :
            idx(idx_), pass(pass_), cmd(cmd_)
        {}
    };


    template <typename Walker>
    FindResult ctr_find_fw(
            const TreePathT& start_path,
            TreePathT& end_path,
            int level,
            NodeChain node_chain,
            Walker&& walker,
            WalkDirection direction = WalkDirection::UP
    ) const;



    template <typename Walker>
    FindResult ctr_find_bw(
            const TreePathT& start_path,
            TreePathT& end_path,
            int32_t level,
            NodeChain node_chain,
            Walker&& walker,
            WalkDirection direction = WalkDirection::UP
    ) const;

    template <int32_t Stream>
    IteratorPtr ctr_seek_stream(CtrSizeT position) const
    {
        typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(position);
        return self().ctr_find(walker);
    }




    template <typename Walker>
    void ctr_walk_tree_up(TreeNodeConstPtr node, int32_t idx, Walker&& walker) const
    {
        if (node->is_leaf())
        {
            self().leaf_dispatcher().dispatch(node, walker, WalkCmd::LAST_LEAF, 0, idx).get_or_throw();
        }
        else {
            self().branch_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx).get_or_throw();
        }

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().ctr_get_node_parent(node);

            self().node_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx).get_or_throw();
        }
    }

protected:

    void ctr_check_path_td(const TreePathT& path, int32_t up_to) const
    {
        for (int32_t c = path.size() - 1; c >= up_to; c--) {
            if (!path[c]) {
                MEMORIA_MAKE_GENERIC_ERROR("Null path element at {}", c).do_throw();
            }
        }
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::FindName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::ctr_find_fw(
        const TreePathT& start_path,
        TreePathT& end_path,
        int level,
        NodeChain node_chain,
        Walker&& walker,
        WalkDirection direction
) const
{
    auto& self = this->self();

    auto start       = node_chain.start;
    const auto& node = node_chain.node;

    auto result = self.node_dispatcher().dispatch(node, std::forward<Walker>(walker), direction, start).get_or_throw();
    node_chain.end = result.local_pos();

    if (direction == WalkDirection::UP)
    {
        if (!result.out_of_range())
        {
            if (node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult{result.local_pos(), cmd};
            }
            else {
                auto child = self.ctr_get_node_child(node, result.local_pos());

                end_path.set(level - 1, child);

                return ctr_find_fw(
                            start_path,
                            end_path,
                            level - 1,
                            NodeChain(self, child, 0, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::DOWN
                );
            }
        }
        else {
            if (!node_chain.node->is_root())
            {
                auto parent = self.ctr_get_node_parent(start_path, level);
                auto parent_idx = self.ctr_get_child_idx(parent, node->id());

                auto find_up = ctr_find_fw(
                            start_path,
                            end_path,
                            level + 1,
                            NodeChain(self, parent, parent_idx + 1, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::UP
                );

                if (find_up.pass)
                {
                    return find_up;
                }
            }

            if (node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult{result.local_pos(), cmd};
            }
            else if (!result.empty())
            {
                self.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1).get_or_throw();
                node_chain.end = result.local_pos() - 1;

                auto child = self.ctr_get_node_child(node, result.local_pos() - 1);
                end_path.set(level - 1, child);

                return ctr_find_fw(
                            start_path,
                            end_path,
                            level - 1,
                            NodeChain(self, child, 0, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::DOWN
                );
            }
            else {
                return FindResult{start, WalkCmd::NONE, false};
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        auto cmd = node_chain.processChain(std::forward<Walker>(walker));
        return FindResult{result.local_pos(), cmd};
    }
    else if (!result.out_of_range())
    {
        auto child = self.ctr_get_node_child(node_chain.node, result.local_pos());

        end_path.set(level - 1, child);

        return ctr_find_fw(
                    start_path,
                    end_path,
                    level - 1,
                    NodeChain(self, child, 0, &node_chain),
                    std::forward<Walker>(walker),
                    WalkDirection::DOWN
        );
    }
    else
    {
        self.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1).get_or_throw();
        node_chain.end = result.local_pos() - 1;

        auto child = self.ctr_get_node_child(node_chain.node, result.local_pos() - 1);

        end_path.set(level - 1, child);

        return ctr_find_fw(
                    start_path,
                    end_path,
                    level - 1,
                    NodeChain(self, child, 0, &node_chain),
                    std::forward<Walker>(walker),
                    WalkDirection::DOWN
        );
    }
}




M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::ctr_find_bw(
        const TreePathT& start_path,
        TreePathT& end_path,
        int32_t level,
        NodeChain node_chain,
        Walker&& walker,
        WalkDirection direction
) const
{
    auto& self = this->self();

    auto result = self.node_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), direction, node_chain.start).get_or_throw();
    node_chain.end = result.local_pos();

    const int32_t max = std::numeric_limits<int32_t>::max() - 2;

    if (direction == WalkDirection::UP)
    {
        if (!result.out_of_range())
        {
            if (node_chain.node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult{result.local_pos(), cmd};
            }
            else {
                auto child = self.ctr_get_node_child(node_chain.node, result.local_pos());

                return ctr_find_bw(
                            start_path,
                            end_path,
                            level - 1,
                            NodeChain(self, child, max, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::DOWN
                );
            }
        }
        else {
            if (!node_chain.node->is_root())
            {
                auto parent = self.ctr_get_node_parent(start_path, level);
                auto parent_idx = self.ctr_get_child_idx(parent, node_chain.node->id());

                auto find_up = ctr_find_bw(
                            start_path,
                            end_path,
                            level + 1,
                            NodeChain(self, parent, parent_idx - 1, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::UP
                );

                if (find_up.pass)
                {
                    return find_up;
                }
            }

            if (node_chain.node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult{result.local_pos(), cmd};
            }
            else if (!result.empty())
            {
                self.branch_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos()).get_or_throw();
                node_chain.end = result.local_pos();

                auto child = self.ctr_get_node_child(node_chain.node, result.local_pos() + 1);

                return ctr_find_bw(
                            start_path,
                            end_path,
                            level - 1,
                            NodeChain(self, child, max, &node_chain),
                            std::forward<Walker>(walker),
                            WalkDirection::DOWN
                );
            }
            else {
                return FindResult{node_chain.start, WalkCmd::NONE, false};
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        auto cmd = node_chain.processChain(std::forward<Walker>(walker));
        return FindResult{result.local_pos(), cmd};
    }
    else if (!result.out_of_range())
    {
        auto child = self.ctr_get_node_child(node_chain.node, result.local_pos());
        return ctr_find_bw(
                    start_path,
                    end_path,
                    level - 1,
                    NodeChain(self, child, max, &node_chain),
                    std::forward<Walker>(walker),
                    WalkDirection::DOWN
        );
    }
    else
    {
        self.branch_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos()).get_or_throw();
        node_chain.end = result.local_pos();

        auto child = self.ctr_get_node_child(node_chain.node, result.local_pos() + 1);

        return ctr_find_bw(
                    start_path,
                    end_path,
                    level - 1,
                    NodeChain(self, child, max, &node_chain),
                    std::forward<Walker>(walker),
                    WalkDirection::DOWN
        );
    }
}





M_PARAMS
template <typename Walker>
typename M_TYPE::IteratorPtr M_TYPE::ctr_find(Walker&& walker) const
{
    auto& self = the_self();

    IteratorPtr i = self.make_iterator();
    i->iter_prepare();

    auto node = self.ctr_get_root_node();

    if (MMA_UNLIKELY(!node->is_root())) {
        MEMORIA_MAKE_GENERIC_ERROR("The node {} is not root", node->id()).do_throw();
    }

    TreePathT& path = i->path();

    path.resize(node->level() + 1);

    int32_t level = node->level();

    std::vector<int32_t> levels;

    if (node.isSet())
    {
        while (level > 0)
        {
            levels.push_back(node->level());
            path.set(level, node);

            auto result = self.branch_dispatcher().dispatch(node, walker, WalkDirection::DOWN, 0).get_or_throw();
            int32_t idx = result.local_pos();

            if (result.out_of_range())
            {
                --idx;
                self.branch_dispatcher().dispatch(node, walker, WalkCmd::FIX_TARGET, 0, idx).get_or_throw();
            }

            self.branch_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx).get_or_throw();

            auto child = self.ctr_get_node_child(node, idx);
            node = child;

            --level;
        }

        levels.push_back(node->level());
        path.set(level, node);

        auto result = self.leaf_dispatcher().dispatch(node, walker, WalkDirection::DOWN, 0).get_or_throw();

        self.leaf_dispatcher().dispatch(node, walker, WalkCmd::LAST_LEAF, 0, result.local_pos()).get_or_throw();

        i->iter_leaf().assign(node);

        walker.finish(*i.get(), result.local_pos(), WalkCmd::LAST_LEAF);
    }

    i->iter_init();
    i->refresh_iovector_view();

    return std::move(i);
}




#undef M_TYPE
#undef M_PARAMS

}
