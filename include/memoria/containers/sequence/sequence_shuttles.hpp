
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

#include <memoria/prototypes/bt/shuttles/bt_find_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_skip_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_select_shuttle.hpp>
#include <memoria/prototypes/bt/shuttles/bt_rank_shuttle.hpp>

namespace memoria {

template<typename Types> class SequenceChunkImpl;

namespace bt {

template <typename Types, typename LeafPath, typename StateTypes>
class FindForwardShuttle<Types, LeafPath, SequenceChunkImpl<StateTypes>, DTOrdering::SUM>: public FindSumForwardShuttleBase<Types, LeafPath> {
    using Base = FindSumForwardShuttleBase<Types, LeafPath>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        iter_state.set_position(last_leaf_pos_, last_leaf_size_);
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


template <typename Types, typename LeafPath, typename StateTypes>
class FindForwardShuttle<Types, LeafPath, SequenceChunkImpl<StateTypes>, DTOrdering::MAX>: public FindMaxForwardShuttleBase<Types, LeafPath> {
    using Base = FindMaxForwardShuttleBase<Types, LeafPath>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        iter_state.set_position(last_leaf_pos_, last_leaf_size_);
    }
};



template <typename Types, typename LeafPath, typename StateTypes>
class FindBackwardShuttle<Types, LeafPath, SequenceChunkImpl<StateTypes>, DTOrdering::SUM>: public FindSumBackwardShuttleBase<Types, LeafPath> {
    using Base = FindSumBackwardShuttleBase<Types, LeafPath>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        iter_state.set_position(last_leaf_pos_, last_leaf_size_, before_start_);
    }


    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);

        auto before_start = iter_state.is_before_start();
        return !before_start ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};




template <typename Types, size_t Stream, typename StateTypes>
class SkipForwardShuttle<Types, Stream, SequenceChunkImpl<StateTypes>>: public SkipForwardShuttleBase<Types, Stream> {
    using Base = SkipForwardShuttleBase<Types, Stream>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        leaf_start_ = iter_state.entry_offset_in_chunk();
    }


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.set_position(last_leaf_pos_, last_leaf_size_);
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




template <typename Types, size_t Stream, typename StateTypes>
class SkipBackwardShuttle<Types, Stream, SequenceChunkImpl<StateTypes>>: public SkipBackwardShuttleBase<Types, Stream> {
    using Base   = SkipBackwardShuttleBase<Types, Stream>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        leaf_start_ = iter_state.entry_offset_in_chunk();
        before_start_ = iter_state.is_before_start();
    }


    virtual void finish(IteratorState& state)
    {
        StateT& iter_state = *static_cast<StateT*>(&state);
        iter_state.set_position(last_leaf_pos_, last_leaf_size_, before_start_);
    }


    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);

        auto before_start = iter_state.is_before_start();
        return !before_start ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};




template <typename Types, typename LeafPath, typename StateTypes>
class SelectForwardShuttle<Types, LeafPath, SequenceChunkImpl<StateTypes>>: public SelectForwardShuttleBase<Types, LeafPath> {
    using Base = SelectForwardShuttleBase<Types, LeafPath>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        iter_state.set_position(last_leaf_pos_, last_leaf_size_);
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




template <typename Types, typename LeafPath, typename StateTypes>
class SelectBackwardShuttle<Types, LeafPath, SequenceChunkImpl<StateTypes>>: public SelectBackwardShuttleBase<Types, LeafPath> {
    using Base   = SelectBackwardShuttleBase<Types, LeafPath>;
    using StateT = SequenceChunkImpl<StateTypes>;

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
        iter_state.set_position(last_leaf_pos_, last_leaf_size_, before_start_);
    }


    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const
    {
        const StateT& iter_state = *static_cast<const StateT*>(&state);

        auto before_start = iter_state.is_before_start();
        return !before_start ? ShuttleEligibility::YES : ShuttleEligibility::NO;
    }
};




}}
