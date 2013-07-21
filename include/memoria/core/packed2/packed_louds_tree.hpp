
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED2_LOUDSTREE_HPP_
#define MEMORIA_CORE_PACKED2_LOUDSTREE_HPP_

#include <memoria/core/packed2/packed_fse_searchable_seq.hpp>

namespace memoria {

class PackedLoudsNode {
	Int idx_;
	Int rank_;

public:
	PackedLoudsNode(): idx_(-1), rank_(0) {}
	PackedLoudsNode(Int idx, Int rank): idx_(idx), rank_(rank) {}

	Int idx() const 	{return idx_;}
	Int node() const 	{return rank_;}

	Int rank0() const
	{
		return idx_ + 1 - rank_;
	}

	Int rank1() const
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

	Int length_;

public:

	PackedLoudsNodeSet():
		Base()
	{}

	PackedLoudsNodeSet(Int idx, Int rank, Int length):
		Base(idx, rank), length_(length)
	{}

	PackedLoudsNodeSet(const PackedLoudsNode& node, Int length):
		Base(node.idx(), node.rank1()), length_(length)
	{}

	Int length() const {
		return length_;
	}

	PackedLoudsNode node(Int idx) const
	{
		return PackedLoudsNode(Base::idx() + idx, Base::rank1() + idx);
	}
};


template <
	Int BF = PackedTreeBranchingFactor,
	Int VPB = 512
>
struct LoudsTreeTypes {
    static const Int BranchingFactor        = BF;
    static const Int ValuesPerBranch        = VPB;
};


template <typename Types>
class PackedLoudsTree: public PackedFSESearchableSeq<
PackedFSESeachableSeqTypes <
	1,
	Types::BranchingFactor,
	Types::ValuesPerBranch
>
> {

	typedef PackedFSESearchableSeq <
			PackedFSESeachableSeqTypes <
				1
			>
	> 																			Base;
	typedef PackedLoudsTree<Types> 												MyType;

	typedef typename Base::Value												Value;

public:
	PackedLoudsTree() {}

	Int writeUDS(Int idx, Int value)
	{
		Int max = idx + value;

		for (; idx < max; idx++)
		{
			this->symbol(idx) = 1;
		}

		this->symbol(idx++) = 0;

		return idx;
	}

	Int insertUDS(Int idx, Int degree)
	{
		this->insertDataRoom(idx, degree + 1);
		return writeUDS(idx, degree);
	}

	Int appendUDS(Int degree)
	{
		return insertUDS(this->size(), degree);
	}

	PackedLoudsNode root() const
	{
		return PackedLoudsNode(0, 1);
	}

	PackedLoudsNode parent(const PackedLoudsNode& node) const
	{
		Int idx = select1(node.rank0());
		return PackedLoudsNode(idx, node.rank0());
	}

	PackedLoudsNode left_sibling(const PackedLoudsNode& node) const
	{
		if (this->symbol(node.idx() - 1) == 1)
		{
			return PackedLoudsNode(node.idx() - 1, node.rank1() - 1);
		}
		else {
			return PackedLoudsNode();
		}
	}

	PackedLoudsNode right_sibling(const PackedLoudsNode& node) const
	{
		if (this->symbol(node.idx() + 1) == 1)
		{
			return PackedLoudsNode(node.idx() + 1, node.rank1() + 1);
		}
		else {
			return PackedLoudsNode();
		}
	}

	PackedLoudsNode first_child(const PackedLoudsNode& node) const
	{
		Int idx = select0(node.rank1()) + 1;
		return PackedLoudsNode(idx, idx + 1 - node.rank1());
	}

	PackedLoudsNode last_child(const PackedLoudsNode& node) const
	{
		Int idx = select0(node.rank1() + 1) - 1;
		return PackedLoudsNode(idx + 1, idx + 1 - node.rank1());
	}

	PackedLoudsNode node(Int idx) const
	{
		return PackedLoudsNode(idx, this->rank1(idx));
	}

	PackedLoudsNodeSet children(const PackedLoudsNode& node) const
	{
		PackedLoudsNode first = this->first_child(node);
		PackedLoudsNode last  = this->last_child(node);

		return PackedLoudsNodeSet(first, last.idx() - first.idx());
	}

	PackedLoudsNode insertNode(const PackedLoudsNode& at)
	{
//		this->ensureCapacity(2);

		this->insert(at.idx(), 1, 1);
		this->reindex();

		PackedLoudsNode node = this->node(at.idx());

		Int zero_idx = first_child(node).idx();

		this->insert(zero_idx, 0, 1);

		this->reindex();

		return node;
	}

	void insert(Int idx, BigInt bits, Int nbits)
	{
		this->insertDataRoom(idx, nbits);

		Value* values = this->symbols();

		SetBits(values, idx, bits, nbits);
	}

	bool isLeaf(const PackedLoudsNode& node) const
	{
		return this->symbol(node.idx()) == 0;
	}

	Int rank0(Int pos) const
	{
		return Base::rank(pos + 1, 0);
	}

	Int rank1(Int pos) const
	{
		return Base::rank(pos + 1, 1);
	}

	Int rank1() const
	{
		return Base::rank(this->size(), 1);
	}

	Int select0(Int rank) const
	{
		return Base::selectFw(0, rank).idx();
	}

	Int select1(Int rank) const
	{
		return Base::selectFw(1, rank).idx();
	}

	PackedLoudsNode select1Fw(const PackedLoudsNode& node, Int distance) const
	{
		Int idx = select1(node.rank1() + distance);
		return PackedLoudsNode(idx, node.rank1() + distance);
	}

	PackedLoudsNode select1Bw(const PackedLoudsNode& node, Int distance) const
	{
		Int idx = select1(node.rank1() - (distance - 1));
		return PackedLoudsNode(idx, node.rank1() - (distance - 1));
	}

	PackedLoudsNode next(const PackedLoudsNode& node) const
	{
		Int bit = this->value(node.idx() + 1);
		return PackedLoudsNode(node.idx() + 1, node.rank1() + bit);
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

//	LoudsTree getSubtree(size_t node) const
//	{
//		Int tree_size = 0;
//
//		this->traverseSubtree(node, [&tree_size](const PackedLoudsNode& left, const PackedLoudsNode& right, Int level) {
//			if (left.idx() <= right.idx())
//			{
//				tree_size += right.idx() - left.idx() + 1;
//			}
//		});
//
//		LoudsTree tree(tree_size);
//
//		this->traverseSubtree(node, [&tree, this](const PackedLoudsNode& left, const PackedLoudsNode& right, Int level) {
//			if (left.idx() <= right.idx())
//			{
//				if (left.idx() < right.idx())
//				{
//					auto src = this->source(left, right - left);
//					tree.append(src);
//				}
//
//				tree.appendUDS(0);
//			}
//		});
//
//		tree.reindex();
//
//		return tree;
//	}

private:

	template <typename Functor>
	void traverseSubtree(
			const PackedLoudsNode& left_node,
			const PackedLoudsNode& right_node,
			Functor&& fn, Int level = 0
	) const
	{
		const MyType& tree = *this;

		fn(left_node, next(right_node), level);

		bool left_leaf 		= tree.isLeaf(left_node);
		bool right_leaf		= tree.isLeaf(right_node);

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

			if (left_tgt.idx() < tree.size())
			{
				PackedLoudsNode left_child 	= tree.first_child(left_tgt);
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
			Int level = 0
	) const
	{
		const MyType& tree = *this;

		bool left_leaf 		= tree.isLeaf(left_node);
		bool right_leaf		= tree.isLeaf(right_node);

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

			if (left_tgt.idx() < tree.size())
			{
				PackedLoudsNode left_child 	= tree.first_child(left_tgt);
				PackedLoudsNode right_child = tree.last_child(right_tgt);

				traverseSubtreeReverse(left_child, right_child, fn, level + 1);
			}
		}

		fn(left_node, next(right_node), level);
	}

	bool is_end(const PackedLoudsNode& left_node, const PackedLoudsNode& right_node) const
	{
		return left_node.idx() >= right_node.idx();
	}
};

}


#endif
