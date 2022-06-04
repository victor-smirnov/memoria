
// Copyright 2022 Victor Smirnov
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

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>
#include <memoria/prototypes/bt/shuttles/bt_shuttle_pkd_ops.hpp>

namespace memoria::bt {

template <typename Types, size_t Stream, typename StateType, typename SelectorTag = EmptyType>
class SkipForwardShuttle;

template <typename Types, size_t Stream, typename StateTypes, typename SelectorTag = EmptyType>
class SkipBackwardShuttle;

template <typename Types, size_t Stream>
class SkipForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using LeafPath = IntList<Stream>;

    using CtrSizeT = typename Types::CtrSizeT;
    using Base::is_descending;

    CtrSizeT target_;
    CtrSizeT sum_{};

    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    SkipForwardShuttleBase(CtrSizeT target):
        target_(target)
    {}


    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(0);

        return node.template processStream<BranchStream>(
            PkdFindSumFwFn(SearchType::GT),
            column,
            start,
            target_,
            sum_
        );
    }

    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;

        if (cmd == WalkCmd::FIX_TARGET) {
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto tree = node.template substream<BranchStream>();
            sum_ -= tree.access(column, end);
        }
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto tree = node.template substream<LeafPath>();

        auto tgt = target_ - sum_;
        if (leaf_start_ + tgt < tree.size())
        {
            sum_ = target_;
            last_leaf_pos_ = leaf_start_ + tgt;
            last_leaf_size_ = tree.size();

            leaf_start_ = CtrSizeT{};

            return ShuttleOpResult::found_leaf();
        }
        else {
            last_leaf_size_ = last_leaf_pos_ = tree.size();

            sum_ += tree.size() - leaf_start_;

            leaf_start_ = CtrSizeT{};

            return ShuttleOpResult::not_found();
        }
    }
};








template <typename Types, size_t Stream>
class SkipBackwardShuttleBase: public BackwardShuttleBase<Types> {
    using Base = BackwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using LeafPath = IntList<Stream>;

    using CtrSizeT = typename Types::CtrSizeT;
    using Base::is_descending;
    using Base::branch_nodes_;

    CtrSizeT target_;
    CtrSizeT sum_{};

    bool before_start_{};
    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};


public:
    SkipBackwardShuttleBase(CtrSizeT target):
        target_(target)
    {}


    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(0);

        return node.template processStream<BranchStream>(
            PkdFindSumBwFn(SearchType::GE),
            column,
            start,
            target_,
            sum_
        );
    }

    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;

        if (cmd == WalkCmd::FIX_TARGET) {
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(0);

            auto tree = node.template substream<BranchPath>();
            sum_ -= tree.access(column, end);
        }
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto tree = node.template substream<LeafPath>();

        last_leaf_size_ = tree.size();
        before_start_ = false;

        auto tgt = target_ - sum_;

        if (is_descending()) {
            leaf_start_ = tree.size() - 1;
        }

        if (leaf_start_)
        {
            if (leaf_start_ >= tgt)
            {
                sum_            = target_;
                last_leaf_pos_  = leaf_start_ - tgt;

                return ShuttleOpResult::found_leaf();
            }
            else {
                last_leaf_pos_  = 0;
                before_start_ = true;

                sum_ += leaf_start_ + 1;

                return ShuttleOpResult::not_found();
            }
        }
        else if (tgt)
        {
            sum_++;
            last_leaf_pos_ = CtrSizeT{};
            before_start_ = true;

            return ShuttleOpResult::not_found();
        }
        else {
            last_leaf_pos_ = CtrSizeT{};
            return ShuttleOpResult::found_leaf();
        }
    }
};



template <typename Types, size_t Stream, typename StateT, typename SelectorTag>
class SkipForwardShuttle: public SkipForwardShuttleBase<Types, Stream> {
    using Base = SkipForwardShuttleBase<Types, Stream>;

protected:

    using typename Base::CtrSizeT;
    using typename Base::IteratorState;
    using typename Base::LeafPath;
    using typename Base::LeafNodeTypeSO;

    using Base::leaf_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;
public:
    SkipForwardShuttle(CtrSizeT target): Base(target) {}

    virtual void start(const IteratorState& state)
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        leaf_start_ = iter_state.iter_leaf_position();
    }


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.finish_ride(last_leaf_pos_, last_leaf_size_, false);
    }


    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        auto tree = node.template substream<LeafPath>();

        auto pos = iter_state.entry_offset_in_chunk();
        auto size = tree.size();

        return pos < size ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};






template <typename Types, size_t Stream, typename StateT, typename SelectorTag>
class SkipBackwardShuttle: public SkipBackwardShuttleBase<Types, Stream> {
    using Base   = SkipBackwardShuttleBase<Types, Stream>;

protected:

    using typename Base::CtrSizeT;
    using typename Base::IteratorState;
    using typename Base::LeafPath;
    using typename Base::LeafNodeTypeSO;

    using Base::leaf_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;
    using Base::before_start_;
public:
    SkipBackwardShuttle(CtrSizeT target): Base(target) {}

    virtual void start(const IteratorState& state)
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        leaf_start_ = iter_state.iter_leaf_position();
        before_start_ = iter_state.is_before_start();
    }


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.finish_ride(last_leaf_pos_, last_leaf_size_, before_start_);
    }


    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);

        auto before_start = iter_state.is_before_start();
        return !before_start ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};






template <typename Types, size_t Stream>
class GlobalLeafPrefixShuttle: public UptreeShuttle<Types> {
    using Base = UptreeShuttle<Types>;
protected:
    using typename Base::BranchNodeTypeSO;
    using CtrSizeT = typename Types::CtrSizeT;

    using LeafPath = IntList<Stream>;

    CtrSizeT prefix_{};

public:
    virtual void treeNode(const BranchNodeTypeSO& node, size_t end)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(0);

        auto ss = node.template substream<BranchPath>();

        prefix_ += ss.sum(column, end);
    }

    const CtrSizeT& prefix(){return prefix_;}
};


}
