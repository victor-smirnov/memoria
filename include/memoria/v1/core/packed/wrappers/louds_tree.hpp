
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/packed/wrappers/symbol_sequence.hpp>

#include <vector>

namespace memoria {
namespace v1 {

class LoudsTree1: public PackedFSESequence<1> {

    typedef LoudsTree1                                                           MyType;
    typedef PackedFSESequence<1>                                                Base;

public:
    static const int32_t END                                                        = static_cast<int32_t>(-1);
    typedef std::pair<int32_t, int32_t>                                                 LevelRange;
    typedef vector<LevelRange>                                                  SubtreeRange;

public:

    LoudsTree1(): Base()
    {}

    LoudsTree1(int32_t capacity): Base(capacity)
    {}

    LoudsTree1(const MyType& other): Base(other)
    {}

    LoudsTree1(MyType&& other): Base(other)
    {}

    int32_t root() const {
        return 0;
    }

    int32_t parent(int32_t idx) const
    {
        int32_t rank = rank0(idx);
        return select1(rank);
    }

    int32_t firstChild(int32_t idx) const
    {
        int32_t idx1 = firstChildNode(idx);

        if ((*this)[idx1] == 1)
        {
            return idx1;
        }
        else {
            return END;
        }
    }

    int32_t firstChildNode(int32_t idx) const
    {
        int32_t rank = rank1(idx);
        int32_t idx1 = select0(rank) + 1;

        return idx1;
    }

    int32_t lastChild(int32_t idx) const
    {
        int32_t idx1 = lastChildNode(idx);

        if ((*this)[idx1] == 1)
        {
            return idx1;
        }
        else {
            return END;
        }
    }

    int32_t lastChildNode(int32_t idx) const
    {
        int32_t rank = rank1(idx) + 1;
        int32_t idx1 = select0(rank) - 1;

        return idx1;
    }



    int32_t nextSibling(int32_t idx) const
    {
        if ((*this)[idx + 1] == 1)
        {
            return idx + 1;
        }
        else {
            return END;
        }
    }

    int32_t prevSibling(int32_t idx) const
    {
        if ((*this)[idx - 1] == 1)
        {
            return idx - 1;
        }
        else {
            return END;
        }
    }

    int32_t appendUDS(int32_t value)
    {
        int32_t idx = Base::size();

        writeUDS(idx, value);

        return value + 1;
    }

    int32_t writeUDS(int32_t idx, int32_t value)
    {
        int32_t max = idx + value;

        for (; idx < max; idx++)
        {
            (*this)[idx] = 1;
        }

        (*this)[idx++] = 0;

        return idx;
    }

    int32_t select1(int32_t rank) const
    {
        return select(rank, 1);
    }

    int32_t select1Fw(int32_t start, int32_t rank) const {
        return selectFw(start, rank, 1);
    }

    int32_t select1Bw(int32_t start, int32_t rank) const {
        return selectBw(start, rank, 1);
    }


    int32_t select0(int32_t rank) const
    {
        return select(rank, 0);
    }

    int32_t select(int32_t rank, int32_t symbol) const
    {
        SelectResult result = Base::sequence_->selectFw(symbol, rank);
        return result.is_found() ? result.local_pos() : END;
    }

    int32_t selectFw(int32_t start, int32_t rank, int32_t symbol) const
    {
        SelectResult result = Base::sequence_->selectFw(start, symbol, rank);
        return result.is_found() ? result.local_pos() : Base::sequence_->size();
    }

    int32_t selectBw(int32_t start, int32_t rank, int32_t symbol) const
    {
        SelectResult result = Base::sequence_->selectBw(start, symbol, rank);
        return result.is_found() ? result.local_pos() : END;
    }


    int32_t rank1(int32_t idx) const
    {
        return Base::sequence_->rank(idx + 1, 1);
    }

    int32_t rank1(int32_t start, int32_t end) const
    {
        return Base::sequence_->rank(start, end + 1, 1);
    }

    int32_t rank0(int32_t idx) const
    {
        return Base::sequence_->rank(idx + 1, 0);
    }

    int32_t rank1() const {
        return sequence_->rank(1);
    }

    int32_t rank0() const {
        return sequence_->rank(0);
    }

    int32_t nodes() const {
        return rank1();
    }


    bool isLeaf(int32_t node) const
    {
        return (*this)[node] == 0;
    }

    bool isNotLeaf(int32_t node) const
    {
        return (*this)[node] != 0;
    }

    auto getSubtree(int32_t node) const
    {
        int32_t tree_size = 0;

        this->traverseSubtree(node, [&tree_size](int32_t left, int32_t right, int32_t level)
        {
            if (left <= right)
            {
                tree_size += right - left + 1;
            }
        });

        MyType tree(tree_size);

        this->traverseSubtree(node, [&tree, this](int32_t left, int32_t right, int32_t level)
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

    int32_t levels(int32_t node = 0) const
    {
        int32_t level_counter = 0;

        this->traverseSubtree(node, [&level_counter](int32_t left, int32_t right, int32_t level) {
            level_counter++;
        });

        return level_counter;
    }

    SubtreeRange getSubtreeRange(int32_t node) const
    {
        SubtreeRange range;

        this->traverseSubtree(node, [&range](int32_t left, int32_t right, int32_t level)    {
            if (left <= right)
            {
                range.push_back(LevelRange(left, right));
            }
        });

        return range;
    }

    int32_t getSubtreeSize(int32_t node) const
    {
        int32_t count = 0;

        this->traverseSubtree(node, [&count, this](int32_t left, int32_t right, int32_t level) {
            if (level == 0) {
                count += this->rank1(left, right - 1);
            }
            else {
                count += this->rank1(left, right);
            }
        });

        return count;
    }

    void insertAt(int32_t tgt_node, const MyType& tree)
    {
        MyType& me = *this;

        bool insert_at_leaf = me.isLeaf(tgt_node);

        tree.traverseSubtree(0, [&](int32_t left, int32_t right, int32_t level)
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


    void removeSubtree(int32_t tgt_node)
    {
        MyType& me = *this;

        if (tgt_node > 0)
        {
            if (me.isNotLeaf(tgt_node))
            {
                me.traverseSubtreeReverse(tgt_node, [&me](int32_t left, int32_t right, int32_t level){
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
    void traverseSubtree(int32_t node, Functor&& fn) const
    {
        traverseSubtree(node, node, fn);
    }

    template <typename Functor>
    void traverseSubtreeReverse(int32_t node, Functor&& fn) const
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

    void checkCapacity(int32_t requested)
    {

    }

    template <typename Functor>
    void traverseSubtree(int32_t left_node, int32_t right_node, Functor&& fn, int32_t level = 0) const
    {
        const MyType& tree = *this;

        fn(left_node, right_node + 1, level);

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            int32_t left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            int32_t right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt < tree.size())
            {
                int32_t left_child  = tree.firstChildNode(left_tgt);
                int32_t right_child     = tree.lastChildNode(right_tgt);

                traverseSubtree(left_child, right_child, fn, level + 1);
            }
        }
    }


    template <typename Functor>
    void traverseSubtreeReverse(int32_t left_node, int32_t right_node, Functor&& fn, int32_t level = 0) const
    {
        const MyType& tree = *this;

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            int32_t left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            int32_t right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt < tree.size())
            {
                int32_t left_child  = tree.firstChildNode(left_tgt);
                int32_t right_child     = tree.lastChildNode(right_tgt);

                traverseSubtreeReverse(left_child, right_child, fn, level + 1);
            }
        }

        fn(left_node, right_node + 1, level);
    }

    bool is_end(int32_t left_node, int32_t right_node) const
    {
        return left_node >= right_node;
    }
};


}}
