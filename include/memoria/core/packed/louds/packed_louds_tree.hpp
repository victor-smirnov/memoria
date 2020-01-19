
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

#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria {

class PackedLoudsNode {
    int32_t idx_;
    int32_t rank_;

public:
    PackedLoudsNode(): idx_(-1), rank_(0) {}
    PackedLoudsNode(int32_t idx, int32_t rank): idx_(idx), rank_(rank) {}

    int32_t local_pos() const     {return idx_;}
    int32_t node() const    {return rank_;}

    int32_t rank0() const
    {
        return idx_ + 1 - rank_;
    }

    int32_t rank1() const
    {
        return rank_;
    }

    bool operator==(const PackedLoudsNode& other) const {
        return idx_ == other.idx_;
    }

    bool operator!=(const PackedLoudsNode& other) const {
        return idx_ != other.idx_;
    }

    bool is_empty() const {
        return idx_ < 0;
    }

    operator bool() const {
        return idx_ >= 0;
    }
};

class PackedLoudsNodeSet: public PackedLoudsNode {

    typedef PackedLoudsNode Base;

    int32_t length_;

public:

    PackedLoudsNodeSet():
        Base()
    {}

    PackedLoudsNodeSet(int32_t idx, int32_t rank, int32_t length):
        Base(idx, rank), length_(length)
    {}

    PackedLoudsNodeSet(const PackedLoudsNode& node, int32_t length):
        Base(node.local_pos(), node.rank1()), length_(length)
    {}

    int32_t length() const {
        return length_;
    }

    PackedLoudsNode node(int32_t idx) const
    {
        return PackedLoudsNode(Base::local_pos() + idx, Base::rank1() + idx);
    }
};


template <
    int32_t BF = PackedTreeBranchingFactor,
    int32_t VPB = 512
>
struct LoudsTreeTypes {
    static const int32_t BranchingFactor        = BF;
    static const int32_t ValuesPerBranch        = VPB;
};


template <typename Types>
class PackedLoudsTree: public PkdFSSeq<PkdFSSeqTypes <1, Types::ValuesPerBranch, PkdFQTreeT<int32_t, 2>>> {

    using Base = PkdFSSeq<PkdFSSeqTypes <1, Types::ValuesPerBranch, PkdFQTreeT<int32_t, 2>>>;

    typedef PackedLoudsTree<Types>                                              MyType;

    typedef typename Base::Value                                                Value;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

public:
    PackedLoudsTree() {}

    int32_t writeUDS(int32_t idx, int32_t value)
    {
        int32_t max = idx + value;

        for (; idx < max; idx++)
        {
            this->symbol(idx) = 1;
        }

        this->symbol(idx++) = 0;

        return idx;
    }

    int32_t insertUDS(int32_t idx, int32_t degree)
    {
        this->insertDataRoom(idx, degree + 1);
        return writeUDS(idx, degree);
    }

    int32_t appendUDS(int32_t degree)
    {
        return insertUDS(this->size(), degree);
    }

    PackedLoudsNode root() const
    {
        return PackedLoudsNode(0, 1);
    }

    PackedLoudsNode parent(const PackedLoudsNode& node) const
    {
        int32_t idx = select1(node.rank0());
        return PackedLoudsNode(idx, node.rank0());
    }

    PackedLoudsNode left_sibling(const PackedLoudsNode& node) const
    {
        if (this->symbol(node.local_pos() - 1) == 1)
        {
            return PackedLoudsNode(node.local_pos() - 1, node.rank1() - 1);
        }
        else {
            return PackedLoudsNode();
        }
    }

    PackedLoudsNode right_sibling(const PackedLoudsNode& node) const
    {
        if (this->symbol(node.local_pos() + 1) == 1)
        {
            return PackedLoudsNode(node.local_pos() + 1, node.rank1() + 1);
        }
        else {
            return PackedLoudsNode();
        }
    }

    PackedLoudsNode first_child(const PackedLoudsNode& node) const
    {
        int32_t idx = select0(node.rank1()) + 1;
        return PackedLoudsNode(idx, idx + 1 - node.rank1());
    }

    PackedLoudsNode last_child(const PackedLoudsNode& node) const
    {
        int32_t idx = select0(node.rank1() + 1) - 1;
        return PackedLoudsNode(idx + 1, idx + 1 - node.rank1());
    }

    PackedLoudsNode node(int32_t idx) const
    {
        return PackedLoudsNode(idx, this->rank1(idx));
    }

    PackedLoudsNodeSet children(const PackedLoudsNode& node) const
    {
        PackedLoudsNode first = this->first_child(node);
        PackedLoudsNode last  = this->last_child(node);

        return PackedLoudsNodeSet(first, last.local_pos() - first.local_pos());
    }

    PackedLoudsNode insertNode(const PackedLoudsNode& at)
    {
        this->insert(at.local_pos(), 1, 1);
        this->reindex();

        PackedLoudsNode node = this->node(at.local_pos());

        int32_t zero_idx = first_child(node).local_pos();

        this->insert(zero_idx, 0, 1);

        this->reindex();

        return node;
    }

    void insert(int32_t idx, int64_t bits, int32_t nbits)
    {
        this->insertDataRoom(idx, nbits);

        Value* values = this->symbols();

        SetBits(values, idx, bits, nbits);
    }

    void removeLeaf(const PackedLoudsNode& at)
    {
        PackedLoudsNode child = first_child(at);
        if (this->symbol(child.local_pos()) == 0)
        {
            Base::remove(child.local_pos(),   child.local_pos() + 1);
            Base::remove(at.local_pos(),      at.local_pos() + 1);

            Base::reindex();
        }
        else {
            throw Exception(MA_SRC, "Can't remove non-leaf node");
        }
    }



    bool isLeaf(const PackedLoudsNode& node) const
    {
        return this->symbol(node.local_pos()) == 0;
    }

    bool isAlone(const PackedLoudsNode& node) const
    {
        MEMORIA_V1_ASSERT(node.local_pos(), <, this->size());

        int32_t idx = node.local_pos();

        int32_t size = (idx < this->size() - 1) ? 3 : 2;

        if (idx > 0)
        {
            int32_t value = GetBits(this->symbols(), idx - 1, size);
            return value == 2;
        }
        else {
            int32_t value = GetBits(this->symbols(), idx, size);
            return value == 1;
        }
    }

    int32_t rank0(int32_t pos) const
    {
        return Base::rank(pos + 1, 0);
    }

    int32_t rank1(int32_t pos) const
    {
        return Base::rank(pos + 1, 1);
    }

    int32_t rank1() const
    {
        return Base::rank(this->size(), 1);
    }

    int32_t tree_size() const {
        return rank1();
    }

    int32_t select0(int32_t rank) const
    {
        return Base::selectFw(0, rank).local_pos();
    }

    int32_t select1(int32_t rank) const
    {
        return Base::selectFw(1, rank).local_pos();
    }

    PackedLoudsNode select1Fw(const PackedLoudsNode& node, int32_t distance) const
    {
        int32_t idx = select1(node.rank1() + distance);
        return PackedLoudsNode(idx, node.rank1() + distance);
    }

    PackedLoudsNode select1Bw(const PackedLoudsNode& node, int32_t distance) const
    {
        int32_t idx = select1(node.rank1() - (distance - 1));
        return PackedLoudsNode(idx, node.rank1() - (distance - 1));
    }

    PackedLoudsNode next(const PackedLoudsNode& node) const
    {
        int32_t bit = this->value(node.local_pos() + 1);
        return PackedLoudsNode(node.local_pos() + 1, node.rank1() + bit);
    }

    template <typename Functor>
    void traverseSubtree(const PackedLoudsNode& node, Functor&& fn) const
    {
        this->traverseSubtree(node, node, fn);
    }

    template <typename Functor>
    void traverseSubtreeReverse(const PackedLoudsNode& node, Functor&& fn) const
    {
        this->traverseSubtreeReverse(node, node, fn);
    }

//  LoudsTree getSubtree(size_t node) const
//  {
//      int32_t tree_size = 0;
//
//      this->traverseSubtree(node, [&tree_size](const PackedLoudsNode& left, const PackedLoudsNode& right, int32_t level) {
//          if (left.local_pos() <= right.local_pos())
//          {
//              tree_size += right.local_pos() - left.local_pos() + 1;
//          }
//      });
//
//      LoudsTree tree(tree_size);
//
//      this->traverseSubtree(node, [&tree, this](const PackedLoudsNode& left, const PackedLoudsNode& right, int32_t level) {
//          if (left.local_pos() <= right.local_pos())
//          {
//              if (left.local_pos() < right.local_pos())
//              {
//                  auto src = this->source(left, right - left);
//                  tree.append(src);
//              }
//
//              tree.appendUDS(0);
//          }
//      });
//
//      tree.reindex();
//
//      return tree;
//  }

private:

    template <typename Functor>
    void traverseSubtree(
            const PackedLoudsNode& left_node,
            const PackedLoudsNode& right_node,
            Functor&& fn, int32_t level = 0
    ) const
    {
        const MyType& tree = *this;

        fn(left_node, next(right_node), level);

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            PackedLoudsNode left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            PackedLoudsNode right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt.local_pos() < tree.size())
            {
                PackedLoudsNode left_child  = tree.first_child(left_tgt);
                PackedLoudsNode right_child = tree.last_child(right_tgt);

                traverseSubtree(left_child, right_child, fn, level + 1);
            }
        }
    }


    template <typename Functor>
    void traverseSubtreeReverse(
            const PackedLoudsNode& left_node,
            const PackedLoudsNode& right_node,
            Functor&& fn,
            int32_t level = 0
    ) const
    {
        const MyType& tree = *this;

        bool left_leaf      = tree.isLeaf(left_node);
        bool right_leaf     = tree.isLeaf(right_node);

        if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
        {
            PackedLoudsNode left_tgt;
            if (left_leaf)
            {
                left_tgt = tree.select1Fw(left_node, 1);
            }
            else {
                left_tgt = left_node;
            }

            PackedLoudsNode right_tgt;
            if (right_leaf)
            {
                right_tgt = tree.select1Bw(right_node, 1);
            }
            else {
                right_tgt = right_node;
            }

            if (left_tgt.local_pos() < tree.size())
            {
                PackedLoudsNode left_child  = tree.first_child(left_tgt);
                PackedLoudsNode right_child = tree.last_child(right_tgt);

                traverseSubtreeReverse(left_child, right_child, fn, level + 1);
            }
        }

        fn(left_node, next(right_node), level);
    }

    bool is_end(const PackedLoudsNode& left_node, const PackedLoudsNode& right_node) const
    {
        return left_node.local_pos() >= right_node.local_pos();
    }
};

}
