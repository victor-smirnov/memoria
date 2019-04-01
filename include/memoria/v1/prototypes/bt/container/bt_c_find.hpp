
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/prototypes/bt/walkers/bt_walker_base.hpp>

#include <memoria/v1/core/container/macros.hpp>

#include <limits>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::FindName)
public:
    using Types = TypesType;

    using Allocator = typename Base::Allocator;

    using typename Base::NodeBaseG;
    using typename Base::Iterator;
    using IteratorPtr = typename Base::IteratorPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;



    using LeafStreamsStructList = typename Types::LeafStreamsStructList;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SizesFn, size_sums, Position);
    Position sizes() const
    {
        NodeBaseG node = self().getRoot();
        return NodeDispatcher::dispatch(node, SizesFn());
    }

protected:

    template <typename Walker>
    IteratorPtr find_(Walker&& walker);

    template <typename LeafPath>
    IteratorPtr find_gt(int32_t index, const TargetType<LeafPath>& key)
    {
        typename Types::template FindGTForwardWalker<Types, LeafPath> walker(index, key);
        return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr find_max_gt(int32_t index, const TargetType<LeafPath>& key)
    {
        typename Types::template FindMaxGTWalker<Types, LeafPath> walker(index, key);
        return self().find_(walker);
    }


    template <typename LeafPath>
    IteratorPtr find_ge(int32_t index, const TargetType<LeafPath>& key)
    {
        typename Types::template FindGEForwardWalker<Types, LeafPath> walker(index, key);
        return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr find_max_ge(int32_t index, const TargetType<LeafPath>& key)
    {
        typename Types::template FindMaxGEWalker<Types, LeafPath> walker(index, key);
        return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr rank_(int32_t index, CtrSizeT pos)
    {
        typename Types::template RankForwardWalker<Types, LeafPath> walker(index, pos);
        return self().find_(walker);
    }

    template <typename LeafPath>
    IteratorPtr select_(int32_t index, CtrSizeT rank)
    {
        typename Types::template SelectForwardWalker<Types, LeafPath> walker(index, rank);
        return self().find_(walker);
    }


    struct NodeChain {
        NodeBaseG node;
        int32_t start;
        int32_t end;
        NodeChain* ref;

        NodeChain(NodeBaseG _node, int32_t _start, NodeChain* _ref = nullptr): node(_node), start(_start), end(0), ref(_ref) {}

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

                LeafDispatcher::dispatch(node, std::forward<Walker>(walker), cmd, start, end);

                return cmd;
            }
            else {
                BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::PREFIXES, start, end);

                return WalkCmd::PREFIXES;
            }
        }
    };

    struct FindResult {
        NodeBaseG   node;
        int32_t         idx;
        bool        pass;
        WalkCmd     cmd;

        explicit FindResult(NodeBaseG _node, int32_t _idx, WalkCmd _cmd, bool _pass = true): node(_node), idx(_idx), pass(_pass), cmd(_cmd) {}
    };

    template <typename Walker>
    bt::StreamOpResult find_fw(NodeBaseG& node, int32_t stream, int32_t idx, Walker&& walker);

    template <typename Walker>
    FindResult find_fw(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);


    template <typename Walker>
    FindResult find_bw(NodeChain node_chain, Walker&& walker, WalkDirection direction = WalkDirection::UP);

    template <int32_t Stream>
    IteratorPtr seek_stream(CtrSizeT position)
    {
        typename Types::template SkipForwardWalker<Types, IntList<Stream>> walker(position);
        return self().find_(walker);
    }




    template <typename Walker>
    void walkUp(NodeBaseG node, int32_t idx, Walker&& walker) const
    {
        if (node->is_leaf())
        {
            LeafDispatcher::dispatch(node, walker, WalkCmd::LAST_LEAF, 0, idx);
        }
        else {
            BranchDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
        }

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().getNodeParent(node);

            NodeDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);
        }
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::FindName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::find_fw(NodeChain node_chain, Walker&& walker, WalkDirection direction)
{
    auto& self = this->self();

    auto start       = node_chain.start;
    const auto& node = node_chain.node;

    auto result = NodeDispatcher::dispatch(node, std::forward<Walker>(walker), direction, start);
    node_chain.end = result.local_pos();

    if (direction == WalkDirection::UP)
    {
        if (!result.out_of_range())
        {
            if (node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult(node, result.local_pos(), cmd);
            }
            else {
                auto child = self.getChild(node, result.local_pos());
                return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
            }
        }
        else {
            if (!node_chain.node->is_root())
            {
                auto parent         = self.getNodeParent(node);
                auto parent_idx     = node->parent_idx() + 1;
                auto parent_result  = find_fw(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

                if (parent_result.pass)
                {
                    return parent_result;
                }
            }

            if (node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult(node, result.local_pos(), cmd);
            }
            else if (!result.empty())
            {
                BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1);
                node_chain.end = result.local_pos() - 1;

                auto child = self.getChild(node, result.local_pos() - 1);
                return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
            }
            else {
                return FindResult(node, start, WalkCmd::NONE, false);
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        auto cmd = node_chain.processChain(std::forward<Walker>(walker));
        return FindResult(node_chain.node, result.local_pos(), cmd);
    }
    else if (!result.out_of_range())
    {
        auto child = self.getChild(node_chain.node, result.local_pos());
        return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
        BranchDispatcher::dispatch(node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, start, result.local_pos() - 1);
        node_chain.end = result.local_pos() - 1;

        auto child = self.getChild(node_chain.node, result.local_pos() - 1);
        return find_fw(NodeChain(child, 0, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}




M_PARAMS
template <typename Walker>
typename M_TYPE::FindResult M_TYPE::find_bw(NodeChain node_chain, Walker&& walker, WalkDirection direction)
{
    auto& self = this->self();

    auto result = NodeDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), direction, node_chain.start);
    node_chain.end = result.local_pos();

    const int32_t max = std::numeric_limits<int32_t>::max() - 2;

    if (direction == WalkDirection::UP)
    {
        if (!result.out_of_range())
        {
            if (node_chain.node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult(node_chain.node, result.local_pos(), cmd);
            }
            else {
                auto child = self.getChild(node_chain.node, result.local_pos());
                return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
            }
        }
        else {
            if (!node_chain.node->is_root())
            {
                auto parent         = self.getNodeParent(node_chain.node);
                auto parent_idx     = node_chain.node->parent_idx() - 1;
                auto parent_result  = find_bw(NodeChain(parent, parent_idx, &node_chain), std::forward<Walker>(walker), WalkDirection::UP);

                if (parent_result.pass)
                {
                    return parent_result;
                }
            }

            if (node_chain.node->is_leaf())
            {
                auto cmd = node_chain.processChain(std::forward<Walker>(walker));
                return FindResult(node_chain.node, result.local_pos(), cmd);
            }
            else if (!result.empty())
            {
                BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos());
                node_chain.end = result.local_pos();

                auto child = self.getChild(node_chain.node, result.local_pos() + 1);
                return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
            }
            else {
                return FindResult(node_chain.node, node_chain.start, WalkCmd::NONE, false);
            }
        }
    }
    else if (node_chain.node->is_leaf())
    {
        auto cmd = node_chain.processChain(std::forward<Walker>(walker));
        return FindResult(node_chain.node, result.local_pos(), cmd);
    }
    else if (!result.out_of_range())
    {
        auto child = self.getChild(node_chain.node, result.local_pos());
        return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }
    else
    {
        BranchDispatcher::dispatch(node_chain.node, std::forward<Walker>(walker), WalkCmd::FIX_TARGET, node_chain.start, result.local_pos());
        node_chain.end = result.local_pos();

        auto child = self.getChild(node_chain.node, result.local_pos() + 1);
        return find_bw(NodeChain(child, max, &node_chain), std::forward<Walker>(walker), WalkDirection::DOWN);
    }

}





M_PARAMS
template <typename Walker>
typename M_TYPE::IteratorPtr M_TYPE::find_(Walker&& walker)
{
    auto& self = this->self();

    IteratorPtr i = self.make_iterator();

    i->prepare();

    NodeBaseG node = self.getRoot();
    if (node.isSet())
    {
        while (!node->is_leaf())
        {
            auto result = BranchDispatcher::dispatch(node, walker, WalkDirection::DOWN, 0);
            int32_t idx = result.local_pos();

            if (result.out_of_range())
            {
                idx--;
                BranchDispatcher::dispatch(node, walker, WalkCmd::FIX_TARGET, 0, idx);
            }

            BranchDispatcher::dispatch(node, walker, WalkCmd::PREFIXES, 0, idx);

            node = self.getChild(node, idx);
        }

        auto result = LeafDispatcher::dispatch(node, walker, WalkDirection::DOWN, 0);

        LeafDispatcher::dispatch(node, walker, WalkCmd::LAST_LEAF, 0, result.local_pos());

        i->leaf().assign(node);

        walker.finish(*i.get(), result.local_pos(), WalkCmd::LAST_LEAF);
    }

    i->init();

    return i;
}




#undef M_TYPE
#undef M_PARAMS

}}
