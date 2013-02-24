
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_LOUDSTREE_HPP
#define MEMORIA_CORE_TOOLS_LOUDSTREE_HPP

#include <memoria/core/tools/symbol_sequence.hpp>

#include <vector>

namespace memoria {

using namespace std;

class LoudsTree: public SymbolSequence<1> {

	typedef LoudsTree 															MyType;
	typedef SymbolSequence<1>													Base;

public:
	static const size_t END														= static_cast<size_t>(-1);
	typedef std::pair<size_t, size_t> 											LevelRange;
	typedef vector<LevelRange>													SubtreeRange;

public:

	LoudsTree(): Base()
	{}

	LoudsTree(size_t capacity): Base(capacity)
	{}

	LoudsTree(const MyType& other): Base(other)
	{}

	LoudsTree(MyType&& other): Base(other)
	{}

	size_t parent(size_t idx) const
	{
		size_t rank = rank0(idx);
		return select1(rank);
	}

	size_t firstChild(size_t idx) const
	{
		size_t idx1 = firstChildNode(idx);

		if ((*this)[idx1] == 1)
		{
			return idx1;
		}
		else {
			return END;
		}
	}

	size_t firstChildNode(size_t idx) const
	{
		size_t rank = rank1(idx);
		size_t idx1 = select0(rank) + 1;

		return idx1;
	}

	size_t lastChild(size_t idx) const
	{
		size_t idx1 = lastChildNode(idx);

		if ((*this)[idx1] == 1)
		{
			return idx1;
		}
		else {
			return END;
		}
	}

	size_t lastChildNode(size_t idx) const
	{
		size_t rank = rank1(idx) + 1;
		size_t idx1 = select0(rank) - 1;

		return idx1;
	}



	size_t nextSibling(size_t idx) const
	{
		if ((*this)[idx + 1] == 1)
		{
			return idx + 1;
		}
		else {
			return END;
		}
	}

	size_t prevSibling(size_t idx) const
	{
		if ((*this)[idx - 1] == 1)
		{
			return idx - 1;
		}
		else {
			return END;
		}
	}

	size_t appendUDS(size_t value)
	{
		size_t idx = Base::size();
		Base::enlarge(value + 1);

		writeUDS(idx, value);

		return value + 1;
	}

	size_t writeUDS(size_t idx, size_t value)
	{
		size_t max = idx + value;

		for (; idx < max; idx++)
		{
			(*this)[idx] = 1;
		}

		(*this)[idx++] = 0;

		return idx;
	}

	size_t select1(size_t rank) const
	{
		return select(rank, 1);
	}

	size_t select1Fw(size_t start, size_t rank) const {
		return selectFw(start, rank, 1);
	}

	size_t select1Bw(size_t start, size_t rank) const {
		return selectBw(start, rank, 1);
	}


	size_t select0(size_t rank) const
	{
		return select(rank, 0);
	}

	size_t select(size_t rank, Int symbol) const
	{
		SelectResult result = Base::sequence_->selectFW(symbol, rank);
		return result.is_found() ? result.idx() : END;
	}

	size_t selectFw(size_t start, size_t rank, Int symbol) const
	{
		SelectResult result = Base::sequence_->selectFW(start, symbol, rank);
		return result.is_found() ? result.idx() : Base::sequence_->size();
	}

	size_t selectBw(size_t start, size_t rank, Int symbol) const
	{
		SelectResult result = Base::sequence_->selectBW(start, symbol, rank);
		return result.is_found() ? result.idx() : END;
	}


	size_t rank1(size_t idx) const
	{
		return Base::sequence_->rank1(idx + 1, 1);
	}

	size_t rank1(size_t start, size_t end) const
	{
		return Base::sequence_->rank1(start, end + 1, 1);
	}

	size_t rank0(size_t idx) const
	{
		return Base::sequence_->rank1(idx + 1, 0);
	}

	size_t rank1() const {
		return sequence_->maxIndex(1);
	}

	size_t rank0() const {
		return sequence_->maxIndex(0);
	}




	bool isLeaf(size_t node) const
	{
		return (*this)[node] == 0;
	}

	bool isNotLeaf(size_t node) const
	{
		return (*this)[node] != 0;
	}

	LoudsTree getSubtree(size_t node) const
	{
		size_t tree_size = 0;

		this->traverseSubtree(node, [&tree_size](size_t left, size_t right, size_t level)
		{
			if (left <= right)
			{
				tree_size += right - left + 1;
			}
		});

		LoudsTree tree(tree_size);

		this->traverseSubtree(node, [&tree, this](size_t left, size_t right, size_t level)
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

	size_t levels(size_t node = 0) const
	{
		size_t level_counter = 0;

		this->traverseSubtree(node, [&level_counter](size_t left, size_t right, size_t level) {
			level_counter++;
		});

		return level_counter;
	}

	SubtreeRange getSubtreeRange(size_t node) const
	{
		SubtreeRange range;

		this->traverseSubtree(node, [&range](size_t left, size_t right, size_t level)	{
			if (left <= right)
			{
				range.push_back(LevelRange(left, right));
			}
		});

		return range;
	}

	size_t getSubtreeSize(size_t node) const
	{
		size_t count = 0;

		this->traverseSubtree(node, [&count, this](size_t left, size_t right, size_t level) {
			if (level == 0) {
				count += this->rank1(left, right - 1);
			}
			else {
				count += this->rank1(left, right);
			}
		});

		return count;
	}

	void insertAt(size_t tgt_node, const LoudsTree& tree)
	{
		MyType& me = *this;

		bool insert_at_leaf = me.isLeaf(tgt_node);

		tree.traverseSubtree(0, [&](size_t left, size_t right, size_t level)
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


	void removeSubtree(size_t tgt_node)
	{
		MyType& me = *this;

		if (tgt_node > 0)
		{
			if (me.isNotLeaf(tgt_node))
			{
				me.traverseSubtreeReverse(tgt_node, [&me](size_t left, size_t right, size_t level){
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
	void traverseSubtree(size_t node, Functor&& fn) const
	{
		traverseSubtree(node, node, fn);
	}

	template <typename Functor>
	void traverseSubtreeReverse(size_t node, Functor&& fn) const
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
		LoudsTree tree;

		tree.append(src);

		tree.reindex();

		tree.dump();

		src.reset();
	}

	void checkCapacity(size_t requested)
	{
		if (this->ensureCapacity(requested))
		{
			this->reindex();
		}
	}

	template <typename Functor>
	void traverseSubtree(size_t left_node, size_t right_node, Functor&& fn, size_t level = 0) const
	{
		const MyType& tree = *this;

		fn(left_node, right_node + 1, level);

		bool left_leaf 		= tree.isLeaf(left_node);
		bool right_leaf		= tree.isLeaf(right_node);

		if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
		{
			size_t left_tgt;
			if (left_leaf)
			{
				left_tgt = tree.select1Fw(left_node, 1);
			}
			else {
				left_tgt = left_node;
			}

			size_t right_tgt;
			if (right_leaf)
			{
				right_tgt = tree.select1Bw(right_node, 1);
			}
			else {
				right_tgt = right_node;
			}

			if (left_tgt < tree.size())
			{
				size_t left_child 	= tree.firstChildNode(left_tgt);
				size_t right_child 	= tree.lastChildNode(right_tgt);

				traverseSubtree(left_child, right_child, fn, level + 1);
			}
		}
	}


	template <typename Functor>
	void traverseSubtreeReverse(size_t left_node, size_t right_node, Functor&& fn, size_t level = 0) const
	{
		const MyType& tree = *this;

		bool left_leaf 		= tree.isLeaf(left_node);
		bool right_leaf		= tree.isLeaf(right_node);

		if (!left_leaf || !right_leaf || !is_end(left_node, right_node))
		{
			size_t left_tgt;
			if (left_leaf)
			{
				left_tgt = tree.select1Fw(left_node, 1);
			}
			else {
				left_tgt = left_node;
			}

			size_t right_tgt;
			if (right_leaf)
			{
				right_tgt = tree.select1Bw(right_node, 1);
			}
			else {
				right_tgt = right_node;
			}

			if (left_tgt < tree.size())
			{
				size_t left_child 	= tree.firstChildNode(left_tgt);
				size_t right_child 	= tree.lastChildNode(right_tgt);

				traverseSubtreeReverse(left_child, right_child, fn, level + 1);
			}
		}

		fn(left_node, right_node + 1, level);
	}

	bool is_end(size_t left_node, size_t right_node) const
	{
		return left_node >= right_node;
	}
};


}


#endif
