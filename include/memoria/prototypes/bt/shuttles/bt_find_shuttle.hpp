
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

namespace memoria::bt {



template <typename Types>
class FindForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;
protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
public:
};


template <typename Types, typename LeafPath, DTOrdering SearchType = Types::template KeyOrderingType<LeafPath>>
class FindForwardShuttle;


template <typename Types, typename LeafPath>
class FindForwardShuttle<Types, LeafPath, DTOrdering::SUM>: public FindForwardShuttleBase<Types> {
    using Base = FindForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;

    static constexpr int32_t Streams = Types::Streams;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::ctr_;
    using Base::is_descending;

    KeyType target_;
    size_t column_;
    SearchType search_type_;

    KeyType sum_{};

public:
    FindForwardShuttle(KeyType target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        return node.template processStream<BranchPath>(
            PkdFindSumFwFn(search_type_),
            column_,
            start,
            target_,
            sum_
        );
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node) {
        return node.template processStream<LeafPath>(
            PkdFindSumFwFn(search_type_),
            column_,
            0,
            target_,
            sum_
        );
    }
};




template <typename Types, typename LeafPath>
class FindForwardShuttle<Types, LeafPath, DTOrdering::MAX>: public FindForwardShuttleBase<Types> {
    using Base = FindForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;

    static constexpr int32_t Streams = Types::Streams;

    using KeyType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::is_descending;

    KeyType target_;
    size_t column_;
    SearchType search_type_;

public:
    FindForwardShuttle(KeyType target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual ShuttleOpResult treeNode(const BranchNodeTypeSO& node, size_t)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        return node.template processStream<BranchPath>(
            PkdFindMaxFwFn(search_type_),
            column_,
            target_
        );
    }

    virtual ShuttleOpResult treeNode(const LeafNodeTypeSO& node) {
        return node.template processStream<LeafPath>(
            PkdFindMaxFwFn(search_type_),
            column_,
            target_
        );
    }
};


}
