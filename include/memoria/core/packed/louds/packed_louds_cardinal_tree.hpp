
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED2_LOUDSCARDINALTREE_HPP_
#define MEMORIA_CORE_PACKED2_LOUDSCARDINALTREE_HPP_

#include <memoria/core/packed/louds/packed_louds_tree.hpp>
#include <memoria/core/packed/array/packed_fse_bitmap.hpp>

#include <functional>

namespace memoria {

using namespace std;

template <typename Types>
class PackedLoudsCardinalTree: public PackedAllocator {

	typedef PackedAllocator 								Base;
	typedef PackedLoudsCardinalTree<Types> 					MyType;

public:
	static const Int SafetyGap 								= 8;
	static const Int BitsPerLabel							= Types::BitsPerLabel;

	typedef LoudsTreeTypes<>								TreeTypes;
	typedef PackedLoudsTree<TreeTypes>						LoudsTree;

	typedef PackedFSEBitmapTypes<BitsPerLabel>				LabelArrayTypes;
	typedef PackedFSEBitmap<LabelArrayTypes>				LabelArray;

	enum {
		TREE, LABELS
	};

public:
	PackedLoudsCardinalTree() {}

	LoudsTree* tree() {
		return Base::template get<LoudsTree>(TREE);
	}

	const LoudsTree* tree() const {
		return Base::template get<LoudsTree>(TREE);
	}

	LabelArray* labels() {
		return Base::template get<LabelArray>(LABELS);
	}

	const LabelArray* labels() const {
		return Base::template get<LabelArray>(LABELS);
	}

	Int label(const PackedLoudsNode& node) const
	{
		return labels(node.rank1());
	}

	PackedLoudsNode insertNode(const PackedLoudsNode& at, Int label)
	{
		PackedLoudsNode node = tree()->insertNode(at);
		MEMORIA_ASSERT_TRUE(node);
		MEMORIA_ASSERT_TRUE(labels()->insert(node.rank1() - 1, label));

		return node;
	}

	void removeLeaf(const PackedLoudsNode& node)
	{
		Int idx = node.rank1() - 1;
		labels()->removeSpace(idx, 1);
		tree()->removeLeaf(node);
	}

	void init()
	{
		Int block_size = empty_size();

		Base::init(block_size, 2);

		Base::template allocateEmpty<LoudsTree>(TREE);
		Base::template allocateEmpty<LabelArray>(LABELS);
	}

	PackedLoudsNode find_child(const PackedLoudsNode& node, Int label) const
	{
		const LoudsTree* louds = tree();
		PackedLoudsNodeSet children = louds->children(node);

		const LabelArray* labels = this->labels();

		for (Int c = 0; c < children.length(); c++)
		{
			Int node_label = labels->value(children.rank1() + c - 1);

			if (node_label >= label)
			{
				return PackedLoudsNode(children.idx() + c, children.rank1() + c);
			}
		}

		return PackedLoudsNode(children.idx() + children.length(), children.rank1() + children.length() - 1);
	}


	void insert_path(UBigInt path, Int size, function<void (const PackedLoudsNode&, Int label, Int level)> fn)
	{
		LoudsTree* louds = tree();

		PackedLoudsNode node = louds->root();

		Int level = 0;

		while (!louds->isLeaf(node))
		{
			Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

			node = find_child(node, label);

			Int lbl = labels()->value(node.rank1() - 1);

			if (!louds->isLeaf(node) && lbl == label)
			{
				level++;
			}
			else {
				int a = 0; a++;
				break;
			}
		}

		bool first = true;

		if (level < size)
		{
			while (level < size)
			{
				louds = this->tree();

				louds->insert(node.idx(), 1, 2 - first);
				louds->reindex();

				node = louds->node(node.idx()); // refresh

				Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

				labels()->insert(node.rank1() - 1, label);

				louds->reindex();

				MEMORIA_ASSERT(louds, ==, this->tree());

				fn(node, label, level);

				node = louds->first_child(node);

				level++;
				first = false;
			}

			tree()->insert(node.idx(), 0, 1);
			tree()->reindex();
		}
	}

	bool query_path(UBigInt path, Int size, function<void (const PackedLoudsNode&, Int label, Int level)> fn) const
	{
		LoudsTree* louds = tree();

		PackedLoudsNode node = louds->root();

		Int level = 0;

		while (!louds->isLeaf(node))
		{
			Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

			PackedLoudsNode child = find_child(node, label);

			MEMORIA_ASSERT(child.is_empty(), !=, true);

			level++;
		}

		return level < size;
	}

	enum class Status {
		LEAF, NOT_FOUND, OK, FINISH
	};

	Status remove_path(const PackedLoudsNode& node, UBigInt path, Int size, Int level)
	{
		LoudsTree* louds = tree();
		if (!louds->isLeaf(node))
		{
			Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

			PackedLoudsNode child = find_child(node, label);

			if (child.is_empty())
			{
				return Status::NOT_FOUND;
			}
			else {
				Status status = remove_path(child, path, size, level + 1);

				if (status == Status::OK)
				{
					bool alone = louds->isAlone(node);

					if (node.idx() > 0)
					{
						removeLeaf(node);
					}

					if (alone)
					{
						return Status::OK;
					}
					else {
						return Status::FINISH;
					}
				}
				else {
					return status;
				}
			}
		}
		else {
			return Status::OK;
		}
	}

	bool remove_path(UBigInt path, Int size)
	{
		PackedLoudsNode node = tree()->root();
		return remove_path(node, path, size, 0) != Status::NOT_FOUND;
	}

	void prepare()
	{
		tree()->insert(0, 1, 3);
		tree()->reindex();

		labels()->insertSpace(0, 1);
		labels()->value(0) = 0;
	}


	static Int block_size(Int client_area)
	{
		return Base::block_size(client_area, 2);
	}

	static Int empty_size()
	{
		Int tree_block_size 	= PackedAllocator::roundUpBytesToAlignmentBlocks(LoudsTree::empty_size());
		Int labels_block_size 	= PackedAllocator::roundUpBytesToAlignmentBlocks(LabelArray::empty_size());

		Int client_area = tree_block_size + labels_block_size;

		Int bs = MyType::block_size(client_area);
		return bs;
	}

	void dump(ostream& out = cout, bool dump_index = true) const
	{
//		if (dump_index)
//		{
//			Base::dump(out);
//		}

		out<<"Louds Tree: "<<endl;
		tree()->dump(out, dump_index);
		out<<endl;

		out<<"Cardinal Labels: "<<endl;
		labels()->dump(out);
		out<<endl;
	}
};

}


#endif
