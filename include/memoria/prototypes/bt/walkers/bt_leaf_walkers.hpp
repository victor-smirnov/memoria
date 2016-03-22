
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/prototypes/bt/walkers/bt_walker_base.hpp>

namespace memoria {
namespace bt      {


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

    static const Int Streams                                                    = Types::Streams;

    StaticVector<BigInt, Streams> branch_size_prefix_;

    IteratorBranchNodeEntry branch_prefix_;
    IteratorBranchNodeEntry leaf_prefix_;

    bool compute_branch_    = true;
    bool compute_leaf_      = true;

public:

    template <typename LeafPath>
    using AccumItemH = memoria::bt::AccumItem<LeafStructList, LeafPath, IteratorBranchNodeEntry>;

protected:



public:

    LeafWalkerBase()
    {}

    void result() const {}

    void empty(Iterator& iter)
    {
        iter.idx() = 0;
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
    auto branch_index(Int index)
    {
        return AccumItemH<LeafPath>::value(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto branch_index(Int index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(Int index)
    {
        return AccumItemH<LeafPath>::value(index, leaf_prefix_);
    }

    template <typename LeafPath>
    auto leaf_index(Int index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }


    template <typename LeafPath>
    auto total_index(Int index) const
    {
        return AccumItemH<LeafPath>::cvalue(index, branch_prefix_) +
                AccumItemH<LeafPath>::cvalue(index, leaf_prefix_);
    }

    void prepare(Iterator& iter)
    {
        branch_prefix_      = iter.cache().prefixes();
        leaf_prefix_        = iter.cache().leaf_prefixes();
        branch_size_prefix_ = iter.cache().size_prefix();
    }

    BigInt finish(Iterator& iter, Int idx, WalkCmd cmd)
    {
        iter.finish_walking(idx, self(), cmd);

        iter.idx() = idx;

        iter.cache().prefixes()      = branch_prefix_;
        iter.cache().leaf_prefixes() = leaf_prefix_;
        iter.cache().size_prefix()   = branch_size_prefix_;

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

    const StaticVector<BigInt, Streams>& branch_size_prefix() const {
        return branch_size_prefix_;
    }

    StaticVector<BigInt, Streams>& branch_size_prefix() {
        return branch_size_prefix_;
    }



    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename NodeTypes>
    void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
    {}


    template <typename Node, typename... Args>
    void processLeafIteratorBranchNodeEntry(Node* node, IteratorBranchNodeEntry&accum, Args&&... args)
    {
        detail::LeafAccumWalker<
            LeafStructList,
            LeafRangeList,
            LeafRangeOffsetList,
            Node::template StreamStartIdx<0>::Value
        > w;

        Node::Dispatcher::dispatchAll(node->allocator(), w, self(), accum, std::forward<Args>(args)...);
    }

    struct BranchSizePrefix
    {
        template <Int GroupIdx, Int AllocatorIdx, Int ListIdx, typename StreamObj, typename... Args>
        void stream(const StreamObj* obj, MyType& walker, Args&&... args)
        {
            walker.template branch_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };


    struct LeafSizePrefix
    {
        template <Int GroupIdx, Int AllocatorIdx, Int ListIdx, typename StreamObj, typename... Args>
        void stream(const StreamObj* obj, MyType& walker, Args&&... args)
        {
            walker.template leaf_size_prefix<ListIdx>(obj, std::forward<Args>(args)...);
        }
    };


    template <typename Node, typename... Args>
    void processBranchSizePrefix(Node* node, Args&&... args)
    {
        node->template processStreamsStart(BranchSizePrefix(), self(), std::forward<Args>(args)...);
    }

    template <typename Node, typename... Args>
    void processLeafSizePrefix(Node* node, Args&&... args)
    {
        node->template processStreamsStart(LeafSizePrefix(), self(), std::forward<Args>(args)...);
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

    ForwardLeafWalker(Int, Int, BigInt)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
    {
        return Base::treeNode(std::forward<Args>(args)...);
    }


    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
        Int size = node->size();

        if (start < size)
        {
            return StreamOpResult(start, start, false);
        }
        else {
            return StreamOpResult(size, start, true);
        }
    }

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::LeafNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
        return StreamOpResult(0, 0, true);
    }


    template <typename NodeTypes>
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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
    }


    template <Int StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* stream)
    {
        Base::branch_size_prefix()[StreamIdx] += stream->size();
    }

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item)
    {
        const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        for (Int c = 0; c < Size; c++)
        {
            obj->_add(c + From, item[Idx + c]);
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

    BackwardLeafWalker(Int, Int, BigInt)
    {}

    template <typename... Args>
    auto treeNode(Args&&... args) ->
    decltype(std::declval<Base>().treeNode(std::declval<Args>()...))
    {
        return Base::treeNode(std::forward<Args>(args)...);
    }


    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
        if (start >= 0)
        {
            return StreamOpResult(start, start, false);
        }
        else {
            return StreamOpResult(0, 0, true);
        }
    }

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::LeafNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
        return StreamOpResult(0, 0, true);
    }


    template <typename NodeTypes>
    void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, Int start, Int end)
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
    }


    template <Int StreamIdx, typename StreamType>
    void leaf_size_prefix(const StreamType* stream)
    {
        Base::branch_size_prefix()[StreamIdx] -= stream->size();
    }

    template <Int Offset, Int From, Int Size, typename StreamObj, typename AccumItem>
    void leaf_iterator_BranchNodeEntry(const StreamObj* obj, AccumItem& item)
    {
        const Int Idx = Offset - std::remove_reference<decltype(item)>::type::From;

        for (Int c = 0; c < Size; c++)
        {
            obj->_sub(c + From, item[Idx + c]);
        }
    }
};





}
}
