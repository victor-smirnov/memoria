
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

#include <memoria/core/types/algo.hpp>
#include <memoria/core/datatypes/core.hpp>


namespace memoria::bt {


template <
        typename Types,
        typename LeafPath,
        typename StateT,
        typename SelectorTag = EmptyType,
        DTOrdering SearchType = Types::template KeyOrderingType<LeafPath>
>
class FindForwardShuttle;

template <
        typename Types,
        typename LeafPath,
        typename StateT,
        typename SelectorTag = EmptyType,
        DTOrdering SearchType = Types::template KeyOrderingType<LeafPath>
>
class FindBackwardShuttle;



template <typename Types, typename LeafPath>
class FindSumForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    static constexpr int32_t Streams = Types::Streams;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;


    using Base::is_descending;

    KeyType target_;
    KeyType sum_{};

    size_t column_;
    SearchType search_type_;

    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    FindSumForwardShuttleBase(KeyType target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {        
        using BranchStream = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(column_);

        return node.template processStream<BranchStream>(
            PkdFindSumFwFn(search_type_),
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
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(column_);

            auto tree = node.template substream<BranchStream>();
            sum_ -= tree.access(column, end);
        }
    }


    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        auto tree = node.template substream<LeafPath>();
        auto size = tree.size();

        if (leaf_start_ < size)
        {
            auto tgt = target_ - sum_;
            auto result = tree.findForward(search_type_, column_, leaf_start_, tgt);

            if (result.local_pos() < size)
            {
                sum_ = target_;
                last_leaf_pos_ = result.local_pos();
                last_leaf_size_ = tree.size();

                leaf_start_ = CtrSizeT{};

                return ShuttleOpResult::found_leaf();
            }
            else {
                last_leaf_size_ = last_leaf_pos_ = tree.size();
                sum_ += result.prefix();

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
class FindMaxForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    static constexpr int32_t Streams = Types::Streams;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::is_descending;

    KeyType target_;
    size_t column_;
    SearchType search_type_;

    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    FindMaxForwardShuttleBase(KeyType target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(column_);
        return node.template processStream<BranchPath>(
            PkdFindMaxFwFn(search_type_),
            column,
            target_
        );
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node)
    {
        ShuttleOpResult result = node.template processStream<LeafPath>(
            PkdFindMaxFwFn(search_type_),
            column_,
            target_
        );

        last_leaf_pos_ = result.position();

        auto tree = node.template substream<LeafPath>();
        last_leaf_size_ = tree.size();

        return result;
    }
};









template <typename Types, typename LeafPath>
class FindSumBackwardShuttleBase: public BackwardShuttleBase<Types> {
    using Base = BackwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    static constexpr int32_t Streams = Types::Streams;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::ctr_;
    using Base::is_descending;

    KeyType target_;
    size_t column_;
    SearchType search_type_;

    KeyType sum_{};

    bool before_start_{};
    CtrSizeT leaf_start_{};
    CtrSizeT last_leaf_pos_{};
    CtrSizeT last_leaf_size_{};

public:
    FindSumBackwardShuttleBase(KeyType target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(column_);

        return node.template processStream<BranchPath>(
            PkdFindSumBwFn(search_type_),
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
            size_t column = BranchNodeTypeSO::template translateLeafIndexToBranchIndex<LeafPath>(column_);

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

        if (search_type_ == SearchType::GT)
        {
            auto result = tree.findGTBackward(column_, leaf_start_, tgt);
            sum_ += result.prefix();
            return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() <= leaf_start_);
        }
        else {
            auto result = tree.findGEBackward(column_, leaf_start_, tgt);
            sum_ += result.prefix();
            return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() <= leaf_start_);
        }
    }
};






template <typename Types, typename LeafPath, typename StateT, typename SelectorTag>
class FindForwardShuttle<
        Types,
        LeafPath,
        StateT,
        SelectorTag,
        DTOrdering::SUM
>: public FindSumForwardShuttleBase<Types, LeafPath> {
    using Base = FindSumForwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;
    using typename Base::KeyType;

    using Base::leaf_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    FindForwardShuttle(KeyType target, size_t column, SearchType search_type):
        Base(target, column, search_type)
    {}


    virtual void start(const IteratorState& state)
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);
        leaf_start_ = iter_state.entry_offset_in_chunk();
    }


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.finish_ride(last_leaf_pos_, last_leaf_size_);
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
class FindForwardShuttle<
        Types,
        LeafPath,
        StateT,
        SelectorTag,
        DTOrdering::MAX
>: public FindMaxForwardShuttleBase<Types, LeafPath> {
    using Base = FindMaxForwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    FindForwardShuttle(KeyType target, size_t column, SearchType search_type):
        Base(target, column, search_type)
    {}


    virtual void start(const IteratorState& state)
    {}


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.finish_ride(last_leaf_pos_, last_leaf_size_, false);
    }
};



template <typename Types, typename LeafPath, typename StateT, typename SelectorTag>
class FindBackwardShuttle<
        Types,
        LeafPath,
        StateT,
        SelectorTag,
        DTOrdering::SUM
>: public FindSumBackwardShuttleBase<Types, LeafPath> {
    using Base = FindSumBackwardShuttleBase<Types, LeafPath>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
    using typename Base::IteratorState;
    using typename Base::KeyType;

    using Base::leaf_start_;
    using Base::before_start_;
    using Base::last_leaf_pos_;
    using Base::last_leaf_size_;

public:
    FindBackwardShuttle(KeyType target, size_t column, SearchType search_type):
        Base(target, column, search_type)
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
