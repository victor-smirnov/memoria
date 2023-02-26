
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

#include <memoria/core/types/algo.hpp>

namespace memoria::bt {

template <
        typename Types,
        typename LeafPath,
        typename StateT,
        typename SelectorTag = EmptyType
>
class SelectForwardShuttle;

template <
        typename Types,
        typename LeafPath,
        typename StateT,
        typename SelectorTag = EmptyType
>
class SelectBackwardShuttle;



template <typename Types, typename LeafPath>
class SelectForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::is_descending;

    CtrSizeT rank_;
    CtrSizeT sum_{};

    size_t symbol_;
    SeqOpType op_type_;

    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    SelectForwardShuttleBase(CtrSizeT rank, size_t symbol, SeqOpType op_type):
        rank_(rank), symbol_(symbol), op_type_(op_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t symbol = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

        auto tree = node.template substream<BranchStream>();
        auto size = tree.size();

        if (start < size)
        {
            auto rank = rank_ - sum_;
            auto result = tree.find_for_select_fw(start, rank, symbol, op_type_);
            sum_ += result.rank;

            return ShuttleOpResult::non_empty(result.idx, result.idx < size);
        }
        else {
            return ShuttleOpResult::empty();
        }
    }

    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;

        if (cmd == WalkCmd::FIX_TARGET) {
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

            auto tree = node.template substream<BranchStream>();
            auto value = tree.sum_for_rank(end, end + 1, column, op_type_);
            sum_ -= value;
        }
    }


    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto seq = node.template substream<LeafPath>();
        auto size = seq.size();

        if (leaf_start_ < size)
        {
            auto rank = rank_ - sum_;
            auto result = seq.select_fw(leaf_start_, rank, symbol_, op_type_);

            if (result.idx < size)
            {
                sum_ = rank_;
                last_leaf_pos_ = result.idx;
                last_leaf_size_ = seq.size();

                leaf_start_ = CtrSizeT{};

                return ShuttleOpResult::found_leaf();
            }
            else {
                last_leaf_size_ = last_leaf_pos_ = seq.size();
                sum_ += result.rank;

                leaf_start_ = CtrSizeT{};
                return ShuttleOpResult::not_found();
            }
        }
        else {
            return ShuttleOpResult::empty();
        }
    }
};







template <typename Types, typename LeafPath>
class SelectBackwardShuttleBase: public BackwardShuttleBase<Types> {
    using Base = BackwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::is_descending;

    CtrSizeT rank_;
    size_t symbol_;
    SeqOpType op_type_;

    CtrSizeT sum_{};

    bool before_start_{};
    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    SelectBackwardShuttleBase(CtrSizeT rank, size_t symbol, SeqOpType op_type):
        rank_(rank), symbol_(symbol), op_type_(op_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t symbol = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

        auto tree = node.template substream<BranchStream>();
        auto size = tree.size();

        if (start == std::numeric_limits<size_t>::max()) {
            return ShuttleOpResult::empty();
        }
        else {
            if (start >= size) {
                start = size - 1;
            }

            auto rank = rank_ - sum_;
            auto result = tree.find_for_select_bw(start, rank, symbol, op_type_);
            sum_ += result.rank;

            return ShuttleOpResult::non_empty(result.idx, result.idx <= start);
        }

    }

    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;

        if (cmd == WalkCmd::FIX_TARGET) {
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

            auto tree = node.template substream<BranchPath>();
            auto value = tree.sum_for_rank(end, end + 1, column, op_type_);//  .access(column, end);
            sum_ -= value;
        }
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto seq = node.template substream<LeafPath>();

        last_leaf_size_ = seq.size();
        before_start_ = false;

        auto rank = rank_ - sum_;

        if (is_descending()) {
            leaf_start_ = seq.size() - 1;
        }

        auto result = seq.select_bw(leaf_start_, rank, symbol_, op_type_);

        if (result.idx <= leaf_start_)
        {
            last_leaf_pos_ = result.idx;
            sum_ = rank_;
            before_start_ = false;
            return ShuttleOpResult::non_empty(result.idx, true);
        }
        else {
            sum_ += result.rank;
            last_leaf_pos_ = 0;
            before_start_ = true;
            return ShuttleOpResult::non_empty(result.idx, false);
        }
    }
};




template <typename Types, typename LeafPath, typename StateT, typename SelectorTag>
class SelectForwardShuttle: public SelectForwardShuttleBase<Types, LeafPath> {
    using Base = SelectForwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;
    using typename Base::CtrSizeT;

    using Base::leaf_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    SelectForwardShuttle(CtrSizeT rank, size_t symbol, SeqOpType op_type):
        Base(rank, symbol, op_type)
    {}


    virtual void start(const IteratorState& state)
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        leaf_start_ = iter_state.entry_offset_in_chunk();
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




template <typename Types, typename LeafPath, typename StateT, typename SelectorTag>
class SelectBackwardShuttle: public SelectBackwardShuttleBase<Types, LeafPath> {
    using Base   = SelectBackwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;
    using typename Base::CtrSizeT;

    using Base::leaf_start_;
    using Base::before_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    SelectBackwardShuttle(CtrSizeT rank, size_t symbol, SeqOpType op_type):
        Base(rank, symbol, op_type)
    {}


    virtual void start(const IteratorState& state)
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        leaf_start_ = iter_state.entry_offset_in_chunk();
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








}
