
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED2_LOUDSCARDINALTREE_HPP_
#define MEMORIA_CORE_PACKED2_LOUDSCARDINALTREE_HPP_

#include <memoria/core/packed2/packed_louds_tree.hpp>
#include <memoria/core/packed2/packed_fse_bitmap.hpp>

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

	typedef PackedBitVectorTypes<>							LoudsTreeTypes;
	typedef PackedLoudsTree<LoudsTreeTypes>					LoudsTree;

	typedef PackedFSEBitmapTypes<BitsPerLabel>				LabelArrayTypes;
	typedef PackedFSEBitmap<LabelArrayTypes>				LabelArray;

public:
	PackedLoudsCardinalTree() {}

	LoudsTree* tree() {
		return Base::template get<LoudsTree>(0);
	}

	const LoudsTree* tree() const {
		return Base::template get<LoudsTree>(0);
	}

	LabelArray* labels() {
		return Base::template get<LabelArray>(1);
	}

	const LabelArray* labels() const {
		return Base::template get<LabelArray>(1);
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

	void init(Int block_size, Int nodes)
	{
		Base::init(block_size, 2);

		Int bit_size	= 2 * nodes + 1 + SafetyGap;
		Int louds_size  = LoudsTree::block_size(bit_size);

		auto block = Base::allocate(0, louds_size);

		LoudsTree* tree = this->tree();
		tree->init(block.size());

		Int labels_array_size = LabelArray::block_size(nodes);
		block = Base::allocate(1, labels_array_size);

		LabelArray* labels = this->labels();
		labels->init(block.size());
	}

	void init(Int block_size)
	{
		Base::init(block_size, 2);

		Int client_area = this->client_area();

		auto block = Base::allocate(0, client_area/8);
		MEMORIA_ASSERT_TRUE(block);

		LoudsTree* tree = this->tree();
		tree->init(block.size());

		Int labels_array_size = roundDownBytesToAlignmentBlocks(client_area - block.size());

		block = Base::allocate(1, labels_array_size);
		MEMORIA_ASSERT_TRUE(block);

		LabelArray* labels = this->labels();
		labels->init(block.size());
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

		if (path == 0x732304c4) {
			int a = 0; a++;
		}

		if (level < size)
		{
			while (level < size)
			{
				louds = this->tree();

				MEMORIA_ASSERT(louds->insert(node.idx(), 1, 2 - first), ==, true);

				louds->reindex();

				node = louds->node(node.idx()); // refresh

				Int label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

				MEMORIA_ASSERT(labels()->insert(node.rank1() - 1, label), ==, true);

				MEMORIA_ASSERT(louds, ==, this->tree());

				fn(node, label, level);

				node = louds->first_child(node);

				level++;
				first = false;
			}

			MEMORIA_ASSERT(tree()->insert(node.idx(), 0, 1), ==, true);
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

	void prepare()
	{
		tree()->insert(0, 1, 3);
		tree()->reindex();

		labels()->insertSpace(0, 1);
		labels()->value(0) = 0;
	}


	static Int block_size(Int nodes)
	{
		Int bit_size	= 2 * nodes + 1 + SafetyGap;

		Int client_area = roundUpBytesToAlignmentBlocks(LoudsTree::block_size(bit_size))
						  + roundUpBytesToAlignmentBlocks(LabelArray::block_size(nodes));

		return Base::block_size(client_area, 2);
	}

	void dump(ostream& out = cout, bool dump_index = true) const
	{
		if (dump_index)
		{
			Base::dump(out);
		}

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
