
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

namespace memoria::bt {

template <
        typename Types,
        typename LeafPath,
        typename StateT,
        typename SelectorTag = EmptyType
>
class CountForwardShuttle;



template <typename Types, typename LeafPath>
class CountForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using CtrSizeT = typename Types::CtrSizeT;

    static constexpr size_t Stream = ListHead<LeafPath>::Value;

    using LeafSizesPath = IntList<Stream, 0>;

    using Base::is_descending;

    CtrSizeT sum_{};

    size_t symbol_;

    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    CountForwardShuttleBase(size_t symbol):
        symbol_(symbol)
    {}

    CtrSizeT sum() const {return sum_;}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        using BranchSizesStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafSizesPath>;

        size_t symbol = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(symbol_);

        auto tree = node.template substream<BranchStream>();
        auto sizes = node.template substream<BranchSizesStream>();

        auto size = tree.size();
        if (start < size)
        {
            for (size_t c = start; c < size; c++) {
                auto vv = tree.access(symbol, c);
                auto sz = sizes.access(0, c);

                if (sz == vv) {
                    sum_ += vv;
                }
                else {
                    return ShuttleOpResult::non_empty(c, true);
                }
            }

            return ShuttleOpResult::non_empty(size, false);
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
            auto value = tree.access(column, start);
            sum_ -= value;
        }
    }


    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto seq = node.template substream<LeafPath>();
        auto size = seq.size();

        if (leaf_start_ < size)
        {
            auto result = seq.countFW(leaf_start_, symbol_);

            if (leaf_start_ + result.count() < size)
            {
                last_leaf_pos_  = result.count();
                last_leaf_size_ = seq.size();

                leaf_start_ = CtrSizeT{};

                return ShuttleOpResult::found_leaf();
            }
            else {
                last_leaf_size_ = last_leaf_pos_ = seq.size();
                sum_ += result.count();

                leaf_start_ = CtrSizeT{};
                return ShuttleOpResult::not_found();
            }
        }
        else {
            return ShuttleOpResult::empty();
        }
    }
};





template <typename Types, typename LeafPath, typename StateT, typename SelectorTag>
class CountForwardShuttle: public CountForwardShuttleBase<Types, LeafPath> {
    using Base = CountForwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;
    using typename Base::CtrSizeT;

    using Base::leaf_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    CountForwardShuttle(size_t symbol):
        Base(symbol)
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

        auto pos = iter_state.iter_leaf_position();
        auto size = tree.size();

        return pos < size ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};



template <typename Types, typename LeafPath>
class AlcMapRankShuttle: public UptreeShuttle<Types> {
    using Base = UptreeShuttle<Types>;
protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT leaf_end_;
    size_t level_;
    CtrSizeT rank_{};

public:
    AlcMapRankShuttle(CtrSizeT leaf_end, size_t level):
        leaf_end_(leaf_end),
        level_(level)
    {}


    virtual void treeNode(const BranchNodeTypeSO& node, size_t end)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(level_);

        auto ss = node.template substream<BranchPath>();
        rank_ += ss.sum_for_rank(0, end, column, SeqOpType::EQ);
    }

    virtual void treeNode(const LeafNodeTypeSO& node)
    {
        auto ss = node.template substream<LeafPath>();
        rank_ += ss.rank(leaf_end_, level_);
    }

    const CtrSizeT& rank(){return rank_;}
};



}
