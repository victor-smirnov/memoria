
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {
namespace bt {


template <
    typename Types,
    typename MyType
>
class LeafWalkerBase {
protected:
    typedef Iter<typename Types::IterTypes>                                     Iterator;
    typedef typename Types::IteratorBranchNodeEntry                                 IteratorBranchNodeEntry;
    typedef typename Types::LeafStreamsStructList                               LeafStructList;
    typedef typename Types::LeafRangeList                                       LeafRangeList;
    typedef typename Types::LeafRangeOffsetList                                 LeafRangeOffsetList;

    static const int32_t Streams                                                = Types::Streams;

    StaticVector<int64_t, Streams> branch_size_prefix_;

    IteratorBranchNodeEntry branch_prefix_;
    IteratorBranchNodeEntry leaf_prefix_;

    bool compute_branch_    = true;
    bool compute_leaf_      = true;

public:

    template <typename LeafPath>
    using AccumItemH = bt::AccumItem<LeafStructList, LeafPath, IteratorBranchNodeEntry>;

protected:



public:

    LeafWalkerBase()
    {}

    void result() const {}

    void empty(Iterator& iter)
    {
        iter.iter_local_pos() = 0;
    }

    const bool& compute_branch() const {
        return compute_branch_;
    }

    bool& compute_branch() {
        return compute_branch_;
    }

    const bool& compute_leaf() const {
        return compute_leaf_;
    }

    bool& compute_leaf() {
        return compute_leaf_;
    }


    template <typename LeafPath>
    auto branch_index(int32_t index)
    {
        return AccumItemH<LeafPath>::value(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto branch_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(int32_t index)
    {
        return AccumItemH<LeafPath>::value(index, leaf_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }


    template <typename LeafPath>
    auto total_index(int32_t index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_) +
                AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }

    void prepare(Iterator& iter)
    {
        branch_prefix_      = iter.iter_cache().prefixes();
        leaf_prefix_        = iter.iter_cache().leaf_prefixes();
        branch_size_prefix_ = iter.iter_cache().size_prefix();
    }

    int64_t finish(Iterator& iter, int32_t idx, WalkCmd cmd)
    {
        iter.iter_finish_walking(idx, self(), cmd);

        iter.iter_local_pos() = idx;

        iter.iter_cache().prefixes()      = branch_prefix_;
        iter.iter_cache().leaf_prefixes() = leaf_prefix_;
        iter.iter_cache().size_prefix()   = branch_size_prefix_;

        return 0;
    }

    const IteratorBranchNodeEntry& branch_BranchNodeEntry() const {
        return branch_prefix_;
    }

    IteratorBranchNodeEntry& branch_BranchNodeEntry() {
        return branch_prefix_;
    }

    const IteratorBranchNodeEntry& leaf_BranchNodeEntry() const {
        return leaf_prefix_;
    }

    IteratorBranchNodeEntry& leaf_BranchNodeEntry() {
        return leaf_prefix_;
    }

    const StaticVector<int64_t, Streams>& branch_size_prefix() const {
        return branch_size_prefix_;
    }

    StaticVector<int64_t, Streams>& branch_size_prefix() {
        return branch_size_prefix_;
    }



    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}

    template <typename CtrT, typename NodeT>
    void treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end)
    {}


    template <typename Node, typename... Args>
    void processLeafIteratorBranchNodeEntry(Node& node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
        detail::LeafAccumWalker<
            LeafStructList,
            LeafRangeList,
            LeafRangeOffsetList,
            Node::template StreamStartIdx<0>::Value
        > w;

        node.dispatchAll(w, self(), accum, std::forward<Args>(args)...).get_or_throw();
    }

    struct BranchSizePrefix
    {
        template <int32_t GroupIdx, int32_t AllocatorIdx, int32_t ListIdx, typename StreamObj, typename... Args>
        void stream(StreamObj&& obj, MyType& walker, Args&&... args)
        {
            walker.template branch_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };


    struct LeafSizePrefix
    {
        template <int32_t GroupIdx, int32_t AllocatorIdx, int32_t ListIdx, typename StreamObj, typename... Args>
        void stream(StreamObj&& obj, MyType& walker, Args&&... args)
        {
            walker.template leaf_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };


    template <typename Node, typename... Args>
    void processBranchSizePrefix(Node&& node, Args&&... args)
    {
        node.processStreamsStart(BranchSizePrefix(), self(), std::forward<Args>(args)...).get_or_throw();
    }

    template <typename Node, typename... Args>
    void processLeafSizePrefix(Node&& node, Args&&... args)
    {
        node.processStreamsStart(LeafSizePrefix(), self(), std::forward<Args>(args)...).get_or_throw();
    }
};



template <
    typename Types
>
class ForwardLeafWalker: public LeafWalkerBase<Types, ForwardLeafWalker<Types>> {
protected:
    using Base = LeafWalkerBase<Types, ForwardLeafWalker<Types>>;

    using IteratorBranchNodeEntry = typename Base::IteratorBranchNodeEntry;

public:

    ForwardLeafWalker(): Base()
    {}

    ForwardLeafWalker(int32_t, int32_t, int64_t)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
    {
        return Base::treeNode(std::forward<Args>(args)...);
    }


    template <typename CtrT, typename NodeT>
    Result<StreamOpResult> treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start) noexcept
    {
        int32_t size = node->size();

        if (start < size)
        {
            return StreamOpResult(start, start, false);
        }
        else {
            return StreamOpResult(size, start, true);
        }
    }

    template <typename CtrT, typename NodeT>
    Result<StreamOpResult> treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start) noexcept
    {
        return StreamOpResult(0, 0, true);
    }


    template <typename CtrT, typename NodeT>
    VoidResult treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
    {
        auto& self = this->self();

        if (cmd == WalkCmd::THE_ONLY_LEAF)
        {
            self.leaf_BranchNodeEntry() = IteratorBranchNodeEntry();
        }
        else if (cmd == WalkCmd::FIRST_LEAF)
        {
            self.processLeafIteratorBranchNodeEntry(node, this->branch_BranchNodeEntry());
            self.processLeafSizePrefix(node);
        }
        else {
            self.leaf_BranchNodeEntry() = IteratorBranchNodeEntry();
        }

        return VoidResult::of();
    }


    template <int32_t StreamIdx, typename StreamType>
    void leaf_size_prefix(StreamType&& stream)
    {
        Base::branch_size_prefix()[StreamIdx] += stream.size();
    }

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(StreamObj&& obj, AccumItem& item)
    {
        const int32_t Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        for (int32_t c = 0; c < Size; c++)
        {
            obj._add(c + From, item[Idx + c]);
        }
    }
};



template <
    typename Types
>
class BackwardLeafWalker: public LeafWalkerBase<Types, BackwardLeafWalker<Types>> {
protected:
    using Base = LeafWalkerBase<Types, BackwardLeafWalker<Types>>;
    using IteratorBranchNodeEntry = typename Base::IteratorBranchNodeEntry;

public:

    BackwardLeafWalker(): Base()
    {}

    BackwardLeafWalker(int32_t, int32_t, int64_t)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
    {
        return Base::treeNode(std::forward<Args>(args)...);
    }


    template <typename CtrT, typename NodeT>
    Result<StreamOpResult> treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start) noexcept
    {
        if (start >= 0)
        {
            return StreamOpResult(start, start, false);
        }
        else {
            return StreamOpResult(0, 0, true);
        }
    }

    template <typename CtrT, typename NodeT>
    Result<StreamOpResult> treeNode(LeafNodeSO<CtrT, NodeT>& node, WalkDirection direction, int32_t start) noexcept
    {
        return StreamOpResult(0, 0, true);
    }


    template <typename CtrT, typename NodeT>
    VoidResult treeNode(LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
    {
        auto& self = this->self();

        if (cmd == WalkCmd::THE_ONLY_LEAF)
        {
            self.leaf_BranchNodeEntry() = IteratorBranchNodeEntry();
        }
        else if (cmd == WalkCmd::LAST_LEAF)
        {
            self.leaf_BranchNodeEntry() = IteratorBranchNodeEntry();

            self.processLeafIteratorBranchNodeEntry(node, this->branch_BranchNodeEntry());
            self.processLeafSizePrefix(node);
        }

        return VoidResult::of();
    }


    template <int32_t StreamIdx, typename StreamType>
    void leaf_size_prefix(StreamType&& stream)
    {
        Base::branch_size_prefix()[StreamIdx] -= stream->size();
    }

    template <int32_t Offset, int32_t From, int32_t Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(StreamObj&& obj, AccumItem& item)
    {
        const int32_t Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        for (int32_t c = 0; c < Size; c++)
        {
            obj._sub(c + From, item[Idx + c]);
        }
    }
};



}}
