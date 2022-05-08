
// Copyright 2011-2022 Victor Smirnov
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
#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>

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
    using typename Base::BlockIteratorStatePtr;

    using LeafStreamsStructList = typename Types::LeafStreamsStructList;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    MEMORIA_V1_DECLARE_NODE_FN(SizesFn, size_sums);
    Position sizes() const
    {
        auto node = self().ctr_get_root_node();
        return self().node_dispatcher().dispatch(node, SizesFn());
    }

public:

    template <typename Walker>
    IteratorPtr ctr_find(Walker&& walker) const;

    template <typename StateTypeT, typename ShuttleT, typename... Args>
    IterSharedPtr<StateTypeT> ctr_descend(TypeTag<StateTypeT> state_tag, TypeTag<ShuttleT>, Args&&... args) const {
        ShuttleT shuttle(std::forward<Args>(args)...);
        return ctr_descend(state_tag, shuttle);
    }

    template <typename StateTypeT>
    IterSharedPtr<StateTypeT> ctr_next_leaf(const StateTypeT* current) const
    {
        auto& self = this->self();

        IterSharedPtr<StateTypeT> state = self.make_block_iterator_state(TypeTag<StateTypeT>{});
        state->path() = current->path();

        if (self.ctr_get_next_node(state->path(), 0)) {
            state->on_next_leaf();
            return std::move(state);
        }
        else {
            return IterSharedPtr<StateTypeT>{};
        }
    }

    template <typename StateTypeT>
    IterSharedPtr<StateTypeT> ctr_prev_leaf(const StateTypeT* current) const
    {
        auto& self = this->self();

        IterSharedPtr<StateTypeT> state = self.make_block_iterator_state(TypeTag<StateTypeT>{});
        state->path() = current->path();

        if (self.ctr_get_prev_node(state->path(), 0)) {
            state->on_prev_leaf();
            return std::move(state);
        }
        else {
            return IterSharedPtr<StateTypeT>{};
        }
    }


    template <typename ShuttleTypes>
    void ctr_ride_uptree(const TreePathT& path, bt::UptreeShuttle<ShuttleTypes>& shuttle, int32_t level = 0) const
    {
        auto& self = this->self();

        for (int32_t ll = level; ll < path.size(); ll++)
        {
            if (ll)
            {
                auto child_idx = self.ctr_get_child_idx(path[ll], path[ll - 1]->id());
                self.branch_dispatcher().dispatch(path[ll], shuttle, child_idx);
            }
            else {
                self.leaf_dispatcher().dispatch(path.leaf(), shuttle);
            }
        }
    }

private:

    template <typename StateTypeT, typename ShuttleTypesT>
    IterSharedPtr<StateTypeT> ctr_descend(
            TypeTag<StateTypeT> state_tag,
            bt::ForwardShuttleBase<ShuttleTypesT>& shuttle
    ) const;

public:


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

        NodeChain(const MyType& ctr, const TreeNodeConstPtr& node_, int32_t start_ = 0, NodeChain* ref_ = nullptr):
            ctr_(ctr),
            node(node_), start(start_), end(0), ref(ref_)
        {}

//        void swapRanges()
//        {
//            auto tmp = start;
//            end = start;
//            start = tmp;
//        }

        template <typename Walker>
        WalkCmd processChain(Walker&& walker, int32_t leaf_cnt = 0)
        {
            if (node->is_leaf()) {
                leaf_cnt++;
            }

            if (ref) {
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

                ctr_.leaf_dispatcher().dispatch(node, std::forward<Walker>(walker), cmd, start, end);
                return cmd;
            }
            else {
                ctr_.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end);
                return WalkCmd::PREFIXES;
            }
        }
    };

    struct FindResult {
        int32_t     idx;
        bool        pass;
        WalkCmd     cmd;

        explicit FindResult(int32_t idx_, WalkCmd cmd_, bool pass_ = true):
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





    struct RideNodeChain {
        const MyType& ctr_;
        TreeNodeConstPtr node;
        size_t start;
        size_t end;
        RideNodeChain* ref;

        RideNodeChain(const MyType& ctr, const TreeNodeConstPtr& node_, size_t start_ = 0, RideNodeChain* ref_ = nullptr):
            ctr_(ctr),
            node(node_), start(start_), end(0), ref(ref_)
        {}

        template <typename Walker>
        WalkCmd processChain(Walker&& walker, size_t leaf_cnt = 0)
        {
            if (node->is_leaf()) {
                leaf_cnt++;
            }

            if (ref) {
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

                ctr_.leaf_dispatcher().dispatch(node, std::forward<Walker>(walker), cmd);
                return cmd;
            }
            else {
                ctr_.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end);
                return WalkCmd::PREFIXES;
            }
        }
    };


    template <typename ShuttleTypesT>
    struct ForwardRideParameters {
        const TreePathT& start_path;
        TreePathT& end_path;
        bt::ForwardShuttleBase<ShuttleTypesT>& shuttle;
    };

    template <typename ShuttleTypesT>
    struct BackwardRideParameters {
        const TreePathT& start_path;
        TreePathT& end_path;
        bt::BackwardShuttleBase<ShuttleTypesT>& shuttle;
    };

    struct RideResult {
        size_t     idx;
        bool        pass;
        WalkCmd     cmd;

        explicit RideResult(size_t idx_, WalkCmd cmd_, bool pass_ = true):
            idx(idx_), pass(pass_), cmd(cmd_)
        {}

        explicit RideResult(WalkCmd cmd_, bool pass_ = true):
            idx(), pass(pass_), cmd(cmd_)
        {}

        explicit RideResult(bool pass_ = true):
            idx(), pass(pass_), cmd(WalkCmd::NONE)
        {}
    };




    template <typename IterState, typename ShuttleType, typename... Args>
    IterSharedPtr<IterState> ctr_ride_fw(const IterState* current, TypeTag<ShuttleType>, Args&&... args) const
    {
        auto& self = this->self();

        ShuttleType shuttle(std::forward<Args>(args)...);
        shuttle.start(*current);

        bt::ShuttleEligibility eligible = self.leaf_dispatcher().dispatch(
            current->path().leaf(),
            *static_cast<bt::ForwardShuttleBase<typename ShuttleType::Types>*>(&shuttle),
            *current
        );

        if (eligible == bt::ShuttleEligibility::YES)
        {
            IterSharedPtr<IterState> next = self.make_block_iterator_state(TypeTag<IterState>{});
            next->path() = current->path();

            ForwardRideParameters<typename ShuttleType::Types> params{
                current->path(),
                next->path(),
                shuttle
            };

            ctr_ride_fw(RideNodeChain(self, current->path().leaf()), params);

            shuttle.finish(*next);

            return next;
        }
        else {
            return IterSharedPtr<IterState>{};
        }
    }



    template <typename IterState, typename ShuttleType, typename... Args>
    IterSharedPtr<IterState> ctr_ride_bw(const IterState* current, TypeTag<ShuttleType>, Args&&... args) const
    {
        auto& self = this->self();

        ShuttleType shuttle(std::forward<Args>(args)...);
        shuttle.start(*current);

        bt::ShuttleEligibility eligible = self.leaf_dispatcher().dispatch(
            current->path().leaf(),
            *static_cast<bt::BackwardShuttleBase<typename ShuttleType::Types>*>(&shuttle),
            *current
        );

        if (eligible == bt::ShuttleEligibility::YES)
        {
            IterSharedPtr<IterState> next = self.make_block_iterator_state(TypeTag<IterState>{});
            next->path() = current->path();

            BackwardRideParameters<typename ShuttleType::Types> params{
                current->path(),
                next->path(),
                shuttle
            };

            ctr_ride_bw(RideNodeChain(self, current->path().leaf()), params);
            shuttle.finish(*next);

            return next;
        }
        else {
            return IterSharedPtr<IterState>{};
        }
    }

private:

    template <typename ShuttleTypesT>
    RideResult ctr_ride_fw(
            RideNodeChain node_chain,
            ForwardRideParameters<ShuttleTypesT>& params,
            int level = 0
    ) const;


    template <typename ShuttleTypesT>
    RideResult ctr_ride_bw(
            RideNodeChain node_chain,
            BackwardRideParameters<ShuttleTypesT>& params,
            int32_t level = 0
    ) const;


public:



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
            self().leaf_dispatcher().dispatch(node, walker, WalkCmd::LAST_LEAF, 0, idx);
        }
        else {
            self().branch_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
        }

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().ctr_get_node_parent(node);

            self().node_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
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

    auto result = self.node_dispatcher().dispatch(node, std::forward<Walker>(walker), direction, start);
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
                self.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1);
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
        self.branch_dispatcher().dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1);
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

    auto result = self.node_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), direction, node_chain.start);
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
                end_path.set(level - 1, child);

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
                self.branch_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos());
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
        self.branch_dispatcher().dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos());
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

            auto result = self.branch_dispatcher().dispatch(node, walker, WalkDirection::DOWN, 0);
            int32_t idx = result.local_pos();

            if (result.out_of_range())
            {
                --idx;
                self.branch_dispatcher().dispatch(node, walker, WalkCmd::FIX_TARGET, 0, idx);
            }

            self.branch_dispatcher().dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);

            auto child = self.ctr_get_node_child(node, idx);
            node = child;

            --level;
        }

        levels.push_back(node->level());
        path.set(level, node);

        auto result = self.leaf_dispatcher().dispatch(node, walker, WalkDirection::DOWN, 0);

        self.leaf_dispatcher().dispatch(node, walker, WalkCmd::LAST_LEAF, 0, result.local_pos());

        i->iter_leaf().assign(node);

        walker.finish(*i.get(), result.local_pos(), WalkCmd::LAST_LEAF);
    }

    i->iter_init();
    i->refresh_iovector_view();

    return std::move(i);
}




M_PARAMS
template <typename StateTypeT, typename ShuttleTypesT>
IterSharedPtr<StateTypeT> M_TYPE::ctr_descend(
        TypeTag<StateTypeT> state_tag,
        bt::ForwardShuttleBase<ShuttleTypesT>& shuttle
) const
{
    auto& self = the_self();

    shuttle.set_descending(true);

    IterSharedPtr<StateTypeT> state = self.make_block_iterator_state(state_tag);

    auto node = self.ctr_get_root_node();

    if (MMA_UNLIKELY(!node->is_root())) {
        MEMORIA_MAKE_GENERIC_ERROR("The node {} is not root", node->id()).do_throw();
    }

    TreePathT& path = state->path();

    path.resize(node->level() + 1);

    int32_t level = node->level();

    if (node.isSet())
    {
        while (level > 0)
        {
            path.set(level, node);

            auto result = self.branch_dispatcher().dispatch(node, shuttle, 0);
            size_t idx = result.position();

            if (MMA_UNLIKELY(!result.is_found()))
            {
                --idx;
                self.branch_dispatcher().dispatch(node, shuttle, WalkCmd::FIX_TARGET, 0, idx);
            }

            if (!shuttle.is_simple_ride()) {
                self.branch_dispatcher().dispatch(node, shuttle, WalkCmd::PREFIXES, 0, idx);
            }

            auto child = self.ctr_get_node_child(node, idx);
            node = child;

            --level;
        }
        path.set(level, node);

        self.leaf_dispatcher().dispatch(node, shuttle);

        if (!shuttle.is_simple_ride()) {
            self.leaf_dispatcher().dispatch(node, shuttle, WalkCmd::LAST_LEAF);
        }

        shuttle.finish(*state);
    }

    return std::move(state);
}


M_PARAMS
template <typename ShuttleTypesT>
typename M_TYPE::RideResult M_TYPE::ctr_ride_fw(
        RideNodeChain node_chain,
        ForwardRideParameters<ShuttleTypesT>& params,
        int level
) const
{
    auto& self = this->self();

    const auto& node = node_chain.node;

    bt::ShuttleOpResult result;
    if (node->is_leaf()) {
        result = self.leaf_dispatcher().dispatch(node, params.shuttle);
    }
    else {
        result = self.branch_dispatcher().dispatch(node, params.shuttle, node_chain.start);
        node_chain.end = result.position();
    }

    if (!params.shuttle.is_descending())
    {
        // Ascending...
        if (result.is_found())
        {
            if (node->is_leaf())
            {
                if (!params.shuttle.is_simple_ride()) {
                    auto cmd = node_chain.processChain(params.shuttle);
                    return RideResult{cmd};
                }
                else {
                    return RideResult{};
                }
            }
            else {
                auto child = self.ctr_get_node_child(node, result.position());
                params.end_path.set(level - 1, child);

                // Switch to Descending...
                params.shuttle.set_descending(true);

                return ctr_ride_fw(
                            RideNodeChain(self, child, 0, &node_chain),
                            params,
                            level - 1
                );
            }
        }
        else {
            if (!node_chain.node->is_root())
            {
                auto parent = self.ctr_get_node_parent(params.start_path, level);
                auto parent_idx = self.ctr_get_child_idx(parent, node->id());

                // Still Ascending...
                auto find_up = ctr_ride_fw(                            
                            RideNodeChain(self, parent, parent_idx + 1, &node_chain),
                            params,
                            level + 1
                );

                if (find_up.pass) {
                    return find_up;
                }
            }

            if (node->is_leaf())
            {
                if (!params.shuttle.is_simple_ride()) {
                    auto cmd = node_chain.processChain(params.shuttle);
                    return RideResult{cmd};
                }
                else {
                    return RideResult{};
                }
            }
            else if (!result.is_empty())
            {
                self.branch_dispatcher().dispatch(node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, result.position() - 1);
                node_chain.end = result.position() - 1;

                auto child = self.ctr_get_node_child(node, result.position() - 1);
                params.end_path.set(level - 1, child);

                // Swithc to Descending...
                params.shuttle.set_descending(true);

                return ctr_ride_fw(
                            RideNodeChain(self, child, 0, &node_chain),
                            params,
                            level - 1
                );
            }
            else {
                return RideResult{node_chain.start, WalkCmd::NONE, false};
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        // Descending and at the final leaf        
        if (!params.shuttle.is_simple_ride()) {
            WalkCmd cmd = node_chain.processChain(params.shuttle);
            return RideResult{cmd};
        }
        else {
            return RideResult{};
        }
    }
    else if (result.is_found())
    {
        // Descending...
        auto child = self.ctr_get_node_child(node_chain.node, result.position());

        params.end_path.set(level - 1, child);

        return ctr_ride_fw(
                    RideNodeChain(self, child, 0, &node_chain),
                    params,
                    level - 1
        );
    }
    else
    {
        // Continue Descending...
        self.branch_dispatcher().dispatch(node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, result.position() - 1);
        node_chain.end = result.position() - 1;

        auto child = self.ctr_get_node_child(node_chain.node, result.position() - 1);

        params.end_path.set(level - 1, child);

        return ctr_ride_fw(
                    RideNodeChain(self, child, 0, &node_chain),
                    params,
                    level - 1
        );
    }
}


M_PARAMS
template <typename ShuttleTypesT>
typename M_TYPE::RideResult M_TYPE::ctr_ride_bw(
        RideNodeChain node_chain,
        BackwardRideParameters<ShuttleTypesT>& params,
        int32_t level
) const
{
    auto& self = this->self();

    bt::ShuttleOpResult result;

    if (node_chain.node->is_leaf()) {
        result = self.leaf_dispatcher().dispatch(node_chain.node, params.shuttle);
    }
    else {
        result = self.branch_dispatcher().dispatch(node_chain.node, params.shuttle, node_chain.start);
        node_chain.end = result.position();
    }

    const size_t max = std::numeric_limits<size_t>::max() - 2;

    if (!params.shuttle.is_descending())
    {
        // Ascending...
        if (result.is_found())
        {
            if (node_chain.node->is_leaf())
            {
                if (!params.shuttle.is_simple_ride()) {
                    auto cmd = node_chain.processChain(params.shuttle);
                    return RideResult{result.position(), cmd};
                }
                else {
                    return RideResult{};
                }
            }
            else {
                auto child = self.ctr_get_node_child(node_chain.node, result.position());
                params.end_path.set(level - 1, child);

                // Switch to Descending...
                params.shuttle.set_descending(true);

                return ctr_ride_bw(
                            RideNodeChain(self, child, max, &node_chain),
                            params,
                            level - 1
                );
            }
        }
        else {
            // Still Ascending...
            if (!node_chain.node->is_root())
            {
                auto parent = self.ctr_get_node_parent(params.start_path, level);
                auto parent_idx = self.ctr_get_child_idx(parent, node_chain.node->id());

                size_t tgt_parent_idx = parent_idx ? parent_idx - 1 : std::numeric_limits<size_t>::max();

                auto find_up = ctr_ride_bw(
                            RideNodeChain(self, parent, tgt_parent_idx, &node_chain),
                            params,
                            level + 1
                );

                if (find_up.pass) {
                    return find_up;
                }
            }

            if (node_chain.node->is_leaf())
            {
                if (!params.shuttle.is_simple_ride()) {
                    auto cmd = node_chain.processChain(params.shuttle);
                    return RideResult{result.position(), cmd};
                }
                else {
                    return RideResult{};
                }
            }
            else if (!result.is_empty())
            {
                self.branch_dispatcher().dispatch(node_chain.node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, result.position());
                node_chain.end = 0;

                auto child = self.ctr_get_node_child(node_chain.node, 0);
                params.end_path.set(level - 1, child);

                // Switch to Descending...
                params.shuttle.set_descending(true);

                return ctr_ride_bw(
                            RideNodeChain(self, child, max, &node_chain),
                            params,
                            level - 1
                );
            }
            else {
                return RideResult{node_chain.start, WalkCmd::NONE, false};
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        // Descending...
        if (!params.shuttle.is_simple_ride()) {
            auto cmd = node_chain.processChain(params.shuttle);
            return RideResult{result.position(), cmd};
        }
        else {
            return RideResult{};
        }
    }
    else if (result.is_found())
    {
        // Descending...
        auto child = self.ctr_get_node_child(node_chain.node, result.position());
        params.end_path.set(level - 1, child);

        return ctr_ride_bw(
                    RideNodeChain(self, child, max, &node_chain),
                    params,
                    level - 1
        );
    }
    else
    {
        // Descending...
        self.branch_dispatcher().dispatch(node_chain.node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, result.position());
        node_chain.end = 0;

        auto child = self.ctr_get_node_child(node_chain.node, 0);

        return ctr_ride_bw(
                    RideNodeChain(self, child, max, &node_chain),
                    params,
                    level - 1
        );
    }
}



#undef M_TYPE
#undef M_PARAMS

}
