
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


class PkdFindSumFwFn {
    SearchType search_type_;

public:
    PkdFindSumFwFn(SearchType search_type):
        search_type_(search_type)
    {}


    template <int32_t StreamIdx, typename Tree, typename KeyType, typename AccumType>
    StreamOpResult stream(const Tree& tree, size_t column, size_t start, const KeyType& key, AccumType& sum)
    {
        auto size = tree.size();

        if (start < size)
        {
            KeyType k = key - sum;

            auto result = tree.findForward(search_type_, column, start, k);

            sum += result.prefix();

            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }
};


class PkdFindMaxFwFn {
    SearchType search_type_;
public:
    PkdFindMaxFwFn(SearchType search_type):
        search_type_(search_type)
    {}

    template <int32_t StreamIdx, typename Tree, typename KeyType>
    StreamOpResult stream(const Tree& tree, size_t column, size_t start, const KeyType& key)
    {
        auto size = tree.size();

        if (start < size)
        {
            auto result = tree.findForward(search_type_, column, start, key);
            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }
};




template <typename KeyType, typename AccumType = KeyType>
class PkdSelectFwFn {
    SearchType search_type_;
public:
    PkdSelectFwFn(SearchType search_type):
        search_type_(search_type)
    {}


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult stream(const Tree& tree, size_t column, size_t start)
    {
        auto size = tree.size();

        if (start < size)
        {
            KeyType k = target_ - sum_;

            auto result = tree.findForward(search_type_, column, start, k);

            sum_ += result.prefix();

            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }
};





template <typename Types>
class SkipForwardShuttle: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;

protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;

    static constexpr int32_t Streams = Types::Streams;

    using CtrSizeT = typename Types::CtrSizeT;

    using Base::ctr_;
    using Base::is_descending;

    size_t stream_;
    CtrSizeT target_;
    CtrSizeT sum_{};

public:
    SkipForwardShuttle(size_t stream, CtrSizeT target):
        stream_(stream), target_(target)
    {}


    virtual StreamOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        return node.processStreamStart(
            stream_,
            PkdFindSumFwFn<CtrSizeT>(target_, sum_, SearchType::GT),
            start
        );
    }

    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}

    virtual StreamOpResult treeNode(const LeafNodeTypeSO& node, size_t start) {
        return node.processStreamStart(
                    stream_,
                    PkdFindSumFwFn<CtrSizeT>(target_, sum_, SearchType::GT),
                    start
        );
    }

    virtual void treeNode(const LeafNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}
};


template <typename Types>
class FindForwardShuttleBase: public ForwardShuttleBase<Types> {
    using Base = ForwardShuttleBase<Types>;
protected:
    using typename Base::BranchNodeTypeSO;
    using typename Base::LeafNodeTypeSO;
public:

    virtual void treeNode(const LeafNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}
    virtual void treeNode(const BranchNodeTypeSO& node, WalkCmd cmd, size_t start, size_t end) {}

};



template <typename Types, typename LeafPath>
class FindForwardShuttle: public FindForwardShuttleBase<Types> {
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
    FindForwardShuttle(CtrSizeT target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual StreamOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        return node.template processStream<BranchPath>(
            PkdFindSumFwFn<CtrSizeT>(target_, sum_, search_type_),
            column_,
            start
        );
    }

    virtual StreamOpResult treeNode(const LeafNodeTypeSO& node, size_t start) {
        return node.template processStream<LeafPath>(
            PkdFindSumFwFn<CtrSizeT>(target_, sum_, search_type_),
            column_,
            start
        );
    }
};


template <typename Types, typename LeafPath>
class FindMaxForwardShuttle: public FindForwardShuttleBase<Types> {
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

public:
    FindMaxForwardShuttle(CtrSizeT target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual StreamOpResult treeNode(const BranchNodeTypeSO& node, size_t start)
    {
        using BranchPath = typename BranchNodeTypeSO::template BuildBranchPath<LeafPath>;
        return node.template processStream<BranchPath>(
            PkdFindMaxFwFn<CtrSizeT>(target_, search_type_),
            column_,
            start
        );
    }

    virtual StreamOpResult treeNode(const LeafNodeTypeSO& node, size_t start) {
        return node.template processStream<LeafPath>(
            PkdFindMaxFwFn<CtrSizeT>(target_, search_type_),
            column_,
            start
        );
    }
};



template <typename Types, typename LeafPath>
class SelectForwardShuttle: public FindForwardShuttle<Types, LeafPath> {
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
    SelectForwardShuttle(CtrSizeT target, size_t column, SearchType search_type):
        target_(target), column_(column), search_type_(search_type)
    {}

    virtual StreamOpResult treeNode(const LeafNodeTypeSO& node, size_t start) {
        return node.template processStream<LeafPath>(
            PkdSelectFwFn<CtrSizeT>(target_, sum_, column_),
            start
        );
    }
};



}
