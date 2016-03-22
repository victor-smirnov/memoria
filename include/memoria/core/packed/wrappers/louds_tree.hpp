
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/packed/wrappers/symbol_sequence.hpp>

#include <vector>

namespace memoria {

using namespace std;

class LoudsTree1: public PackedFSESequence<1> {

    typedef LoudsTree1                                                           MyType;
    typedef PackedFSESequence<1>                                                Base;

public:
    static const Int END                                                        = static_cast<Int>(-1);
    typedef std::pair<Int, Int>                                                 LevelRange;
    typedef vector<LevelRange>                                                  SubtreeRange;

public:

    LoudsTree1(): Base()
    {}

    LoudsTree1(Int capacity): Base(capacity)
    {}

    LoudsTree1(const MyType& other): Base(other)
    {}

    LoudsTree1(MyType&& other): Base(other)
    {}

    Int root() const {
        return 0;
    }

    Int parent(Int idx) const
    {
        Int rank = rank0(idx);
        return select1(rank);
    }

    Int firstChild(Int idx) const
    {
        Int idx1 = firstChildNode(idx);

        if ((*this)[idx1] == 1)
        {
            return idx1;
        }
        else {
            return END;
        }
    }

    Int firstChildNode(Int idx) const
    {
        Int rank = rank1(idx);
        Int idx1 = select0(rank) + 1;

        return idx1;
    }

    Int lastChild(Int idx) const
    {
        Int idx1 = lastChildNode(idx);

        if ((*this)[idx1] == 1)
        {
            return idx1;
        }
        else {
            return END;
        }
    }

    Int lastChildNode(Int idx) const
    {
        Int rank = rank1(idx) + 1;
        Int idx1 = select0(rank) - 1;

        return idx1;
    }



    Int nextSibling(Int idx) const
    {
        if ((*this)[idx + 1] == 1)
        {
            return idx + 1;
        }
        else {
            return END;
        }
    }

    Int prevSibling(Int idx) const
    {
        if ((*this)[idx - 1] == 1)
        {
            return idx - 1;
        }
        else {
            return END;
        }
    }

    Int appendUDS(Int value)
    {
        Int idx = Base::size();

        writeUDS(idx, value);

        return value + 1;
    }

    Int writeUDS(Int idx, Int value)
    {
        Int max = idx + value;

        for (; idx < max; idx++)
        {
            (*this)[idx] = 1;
        }

        (*this)[idx++] = 0;

        return idx;
    }

    Int select1(Int rank) const
    {
        return select(rank, 1);
    }

    Int select1Fw(Int start, Int rank) const {
        return selectFw(start, rank, 1);
    }

    Int select1Bw(Int start, Int rank) const {
        return selectBw(start, rank, 1);
    }


    Int select0(Int rank) const
    {
        return select(rank, 0);
    }

    Int select(Int rank, Int symbol) const
    {
        SelectResult result = Base::sequence_->selectFw(symbol, rank);
        return result.is_found() ? result.idx() : END;
    }

    Int selectFw(Int start, Int rank, Int symbol) const
    {
        SelectResult result = Base::sequence_->selectFw(start, symbol, rank);
        return result.is_found() ? result.idx() : Base::sequence_->size();
    }

    Int selectBw(Int start, Int rank, Int symbol) const
    {
        SelectResult result = Base::sequence_->selectBw(start, symbol, rank);
        return result.is_found() ? result.idx() : END;
    }


    Int rank1(Int idx) const
    {
        return Base::sequence_->rank(idx + 1, 1);
    }

    Int rank1(Int start, Int end) const
    {
        return Base::sequence_->rank(start, end + 1, 1);
    }

    Int rank0(Int idx) const
    {
        return Base::sequence_->rank(idx + 1, 0);
    }

    Int rank1() const {
        return sequence_->rank(1);
    }

    Int rank0() const {
        return sequence_->rank(0);
    }

    Int nodes() const {
        return rank1();
    }


    bool isLeaf(Int node) const
    {
        return (*this)[node] == 0;
    }

    bool isNotLeaf(Int node) const
    {
        return (*this)[node] != 0;
    }

    auto getSubtree(Int node) const
    {
        Int tree_size = 0;

        this->traverseSubtree(node, [&tree_size](Int left, Int right, Int level)
        {
            if (left <= right)
            {
                tree_size += right - left + 1;
            }
        });

        MyType tree(tree_size);

        this->traverseSubtree(node, [&tree, this](Int left, Int right, Int level)
        {
            if (left <= right)
            {
                if (left < right)
                {
                    auto src = this->source(left, right - left);
                    tree.append(src);
                }

                tree.appendUDS(0);
            }
        });

        tree.reindex();

        return tree;
    }

    Int levels(Int node = 0) const
    {
        Int level_counter = 0;

        this->traverseSubtree(node, [&level_counter](Int left, Int right, Int level) {
            level_counter++;
        });

        return level_counter;
    }

    SubtreeRange getSubtreeRange(Int node) const
    {
        SubtreeRange range;

        this->traverseSubtree(node, [&range](Int left, Int right, Int level)    {
            if (left <= right)
            {
                range.push_back(LevelRange(left, right));
            }
        });

        return range;
    }

    Int getSubtreeSize(Int node) const
    {
        Int count = 0;

        this->traverseSubtree(node, [&count, this](Int left, Int right, Int level) {
            if (level == 0) {
                count += this->rank1(left, right - 1);
            }
            else {
                count += this->rank1(left, right);
            }
        });

        return count;
    }

    void insertAt(Int tgt_node, const MyType& tree)
    {
        MyType& me = *this;

        bool insert_at_leaf = me.isLeaf(tgt_node);

        tree.traverseSubtree(0, [&](Int left, Int right, Int level)
        {
            if (insert_at_leaf)
            {
                if (level == 0)
                {
                    return;
                }
                else if (level == 1)
                {
                    me.remove(tgt_node, 1);
                }
            }

            auto src = tree.source(left, right - left + (left == 0 ? 0 : 1));

            me.checkCapacity(src.getSize());

            me.insert(tgt_node, src);

            me.reindex();

            if (me.isNotLeaf(tgt_node))
            {
                tgt_node = me.firstChildNode(tgt_node);
            }
            else {
                tgt_node = me.select1Fw(tgt_node, 1);

                if (tgt_node < me.size())
                {
                    tgt_node = me.firstChildNode(tgt_node);
                }
            }
        });
    }


    void removeSubtree(Int tgt_node)
    {
        MyType& me = *this;

        if (tgt_node > 0)
        {
            if (me.isNotLeaf(tgt_node))
            {
                me.traverseSubtreeReverse(tgt_node, [&me](Int left, Int right, Int level){
                    me.remove(left, right - left + (level > 0 ? 1 : 0));
                });

                me.reindex();
            }
        }
        else {
            me.clear();
            me.reindex();
        }
    }

    template <typename Functor>
    void traverseSubtree(Int node, Functor&& fn) const
    {
        traverseSubtree(node, node, fn);
    }

    template <typename Functor>
    void traverseSubtreeReverse(Int node, Functor&& fn) const
    {
        traverseSubtreeReverse(node, node, fn);
    }

    void clear() {
        sequence_->size() = 0;
    }

private:

    template <typename T>
    void dumpTmp(ISequenceDataSource<T, 1>& src)
    {
        MyType tree;

        tree.append(src);

        tree.reindex();

        tree.dump();

        src.reset();
    }

    void checkCapacity(Int requested)
    {

    }

    template <typename Functor>
    void traverseSubtree(Int left_node, Int right_node, Functor&& fn, Int level = 0) const
    {
        const MyType& tree = *this;

        fn(left_node, right_node + 1, level);

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            Int left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            Int right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt < tree.size())
            {
                Int left_child  = tree.firstChildNode(left_tgt);
                Int right_child     = tree.lastChildNode(right_tgt);

                traverseSubtree(left_child, right_child, fn, level + 1);
            }
        }
    }


    template <typename Functor>
    void traverseSubtreeReverse(Int left_node, Int right_node, Functor&& fn, Int level = 0) const
    {
        const MyType& tree = *this;

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            Int left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            Int right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt < tree.size())
            {
                Int left_child  = tree.firstChildNode(left_tgt);
                Int right_child     = tree.lastChildNode(right_tgt);

                traverseSubtreeReverse(left_child, right_child, fn, level + 1);
            }
        }

        fn(left_node, right_node + 1, level);
    }

    bool is_end(Int left_node, Int right_node) const
    {
        return left_node >= right_node;
    }
};


}
