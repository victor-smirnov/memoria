
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
#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>

#include <memoria/core/container/macros.hpp>

#include <limits>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::FindName)
public:
    using Types = TypesType;

    using typename Base::TreeNodeConstPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;
    using typename Base::BlockIteratorStatePtr;
    using typename Base::ShuttleTypes;

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

    template <typename StateTypeT, typename ShuttleT, typename... Args>
    IterSharedPtr<StateTypeT> ctr_descend(TypeTag<StateTypeT> state_tag, TypeTag<ShuttleT>, Args&&... args) const {
        ShuttleT shuttle(std::forward<Args>(args)...);
        return ctr_descend(state_tag, shuttle);
    }


    template <typename StateTypeT, template <typename> class ShuttleT, typename... Args>
    IterSharedPtr<StateTypeT> ctr_descend(TypeTag<StateTypeT> state_tag, bt::ShuttleTag<ShuttleT>, Args&&... args) const {
        ShuttleT<ShuttleTypes> shuttle(std::forward<Args>(args)...);
        return ctr_descend(state_tag, shuttle);
    }

    template <typename StateTypeT>
    IterSharedPtr<StateTypeT> ctr_next_leaf(const StateTypeT* current) const
    {
        auto& self = this->self();

        IterSharedPtr<StateTypeT> state = self.make_block_iterator_state(TypeTag<StateTypeT>{});
        state->prepare_ride(*current);
        auto tmp = current->prepare_next_leaf();
        if (self.ctr_get_next_node(state->path(), 0)) {
            state->on_next_leaf(tmp);
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
        state->prepare_ride(*current);
        auto tmp = current->prepare_prev_leaf();

        if (self.ctr_get_prev_node(state->path(), 0)) {
            state->on_prev_leaf(tmp);
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
    ) const {
        IterSharedPtr<StateTypeT> state = self().make_block_iterator_state(state_tag);
        return memoria_static_pointer_cast<StateTypeT>(ctr_descend(std::move(state), shuttle));
    }

    template <typename ShuttleTypesT>
    BlockIteratorStatePtr ctr_descend(
            BlockIteratorStatePtr&& iter,
            bt::ForwardShuttleBase<ShuttleTypesT>& shuttle
    ) const;

public:


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
        ShuttleType shuttle(std::forward<Args>(args)...);
        return ctr_ride_fw(current, shuttle);
    }


    template <typename IterState, typename ShuttleType>
    IterSharedPtr<IterState> ctr_ride_fw(const IterState* current, ShuttleType& shuttle) const
    {
        auto& self = this->self();

        shuttle.start(*current);

        bt::ShuttleEligibility eligible = self.leaf_dispatcher().dispatch(
            current->path().leaf(),
            *static_cast<bt::ForwardShuttleBase<typename ShuttleType::Types>*>(&shuttle),
            *current
        );

        if (eligible == bt::ShuttleEligibility::YES)
        {
            IterSharedPtr<IterState> next = self.make_block_iterator_state(TypeTag<IterState>{});
            next->prepare_ride(*current);

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
        ShuttleType shuttle(std::forward<Args>(args)...);
        return ctr_ride_bw(current, shuttle);
    }


    template <typename IterState, typename ShuttleType>
    IterSharedPtr<IterState> ctr_ride_bw(const IterState* current, ShuttleType& shuttle) const
    {
        auto& self = this->self();

        shuttle.start(*current);

        bt::ShuttleEligibility eligible = self.leaf_dispatcher().dispatch(
            current->path().leaf(),
            *static_cast<bt::BackwardShuttleBase<typename ShuttleType::Types>*>(&shuttle),
            *current
        );

        if (eligible == bt::ShuttleEligibility::YES)
        {
            IterSharedPtr<IterState> next = self.make_block_iterator_state(TypeTag<IterState>{});
            next->prepare_ride(*current);

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
template <typename ShuttleTypesT>
typename M_TYPE::BlockIteratorStatePtr M_TYPE::ctr_descend(
        BlockIteratorStatePtr&& state,
        bt::ForwardShuttleBase<ShuttleTypesT>& shuttle
) const
{
    auto& self = the_self();

    shuttle.set_descending(true);

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
                self.branch_dispatcher().dispatch(node_chain.node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, 0);
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
        self.branch_dispatcher().dispatch(node_chain.node, params.shuttle, WalkCmd::FIX_TARGET, node_chain.start, 0);
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
