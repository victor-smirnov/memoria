
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

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>
#include <boost/variant2.hpp>


namespace memoria::bt {

class ShuttleOpResult {

    // Position is only valid for branch nodes,
    // because it's a child index. For leaf nodes
    // position can have arbitrarty value and is managed
    // by Shuttle/IteratorState interaction internally.

    // In case of found_ == false, for forward shuttles,
    // position_ must be size_. For backward shuttles,
    // position_ is not used ans assument to be 0.
    size_t position_;

    bool found_ : 1;
    bool empty_ : 1;

    ShuttleOpResult(size_t position, bool found, bool empty):
        position_(position),        
        found_(found),
        empty_(empty)
    {}

public:

    ShuttleOpResult() {}

    size_t position() const {return position_;}
    bool is_empty() const {return empty_;}
    bool is_found() const {return found_;}

    static ShuttleOpResult found_branch(size_t position) {
        return ShuttleOpResult{position, true, false};
    }

    static ShuttleOpResult found_leaf() {
        return ShuttleOpResult{0, true, false};
    }

    static ShuttleOpResult not_found() {
        return ShuttleOpResult(0, false, false);
    }

    static ShuttleOpResult empty() {
        return ShuttleOpResult(0, false, true);
    }

    static ShuttleOpResult non_empty(size_t position, bool found) {
        return ShuttleOpResult(position, found, true);
    }
};

enum class ShuttleEligibility {
    NO, YES
};

template <typename Types_>
class ForwardShuttleBase {
public:
    using Types = Types_;
protected:

    using LeafNodeTypeSO    = typename Types::LeafNodeSOType;
    using BranchNodeTypeSO  = typename Types::BranchNodeSOType;
    using CtrType           = typename Types::CtrType;
    using IteratorState     = typename Types::IteratorState;

    bool descending_{};
    bool simple_ride_{true};

public:
    virtual ~ForwardShuttleBase() noexcept = default;

    bool is_descending() const {return descending_;}
    void set_descending(bool value) {descending_ = value;}
    bool is_simple_ride() const {return simple_ride_;}



    void init_search() {
    }

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start) = 0;
    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node) = 0;
    virtual void treeNode(const LeafNodeTypeSO& node, WalkCmd cmd) {}

    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const {
        return ShuttleEligibility::YES;
    }

    virtual void start(const IteratorState& state) = 0;
    virtual void finish(IteratorState& state) = 0;
};



template <typename Types_>
class BackwardShuttleBase {
public:
    using Types = Types_;
protected:

    using LeafNodeTypeSO    = typename Types::LeafNodeSOType;
    using BranchNodeTypeSO  = typename Types::BranchNodeSOType;
    using CtrType           = typename Types::CtrType;
    using IteratorState     = typename Types::IteratorState;

    bool descending_{};
    bool simple_ride_{true};

public:
    virtual ~BackwardShuttleBase() noexcept = default;

    bool is_descending() const {return descending_;}
    void set_descending(bool value) {descending_ = value;}
    bool is_simple_ride() const {return simple_ride_;}

    void init_search() {
    }

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start) = 0;
    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node) = 0;
    virtual void treeNode(const LeafNodeTypeSO& node, WalkCmd cmd) {}

    virtual ShuttleEligibility treeNode(const LeafNodeTypeSO& node, const IteratorState& state) const {
        return ShuttleEligibility::YES;
    }

    virtual void start(const IteratorState& state) = 0;
    virtual void finish(IteratorState& state) = 0;
};


template <typename Types>
struct UptreeShuttle {
    using LeafNodeTypeSO    = typename Types::LeafNodeSOType;
    using BranchNodeTypeSO  = typename Types::BranchNodeSOType;

    virtual void treeNode(const BranchNodeTypeSO& node, size_t end) = 0;
    virtual void treeNode(const LeafNodeTypeSO& node) {}
};

}
