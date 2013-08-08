
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_C_UPDATE_HPP
#define MEMORIA_CONTAINERS_LBLTREE_C_UPDATE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/labeled_tree/ltree_names.hpp>
#include <memoria/containers/labeled_tree/ltree_tools.hpp>
#include <memoria/containers/seq_dense/seqd_walkers.hpp>


namespace memoria    {




MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrUpdateName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::LeafDispatcher                                       LeafDispatcher;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Types::PageUpdateMgr										PageUpdateMgr;

	typedef typename Base::Types::LabelsTuple									LabelsTuple;

	static const Int Streams                                                    = Types::Streams;
//
//
//	struct InsertLabelsFn {
//
//		Accumulator& delta_;
//		const LabelsTuple& labels_;
//
//		InsertLabelsFn(Accumulator& delta, const LabelsTuple& labels):
//			delta_(delta),
//			labels_(labels)
//		{}
//
//		template <Int Idx, typename SeqTypes>
//		void stream(PkdFSSeq<SeqTypes>* seq, Int idx) {}
//
//		template <Int Idx, typename StreamTypes>
//		void stream(PackedFSEArray<StreamTypes>* labels, Int idx)
//		{
//			labels->insert(idx, std::get<Idx - 1>(labels_));
//
//			std::get<Idx>(delta_)[0] = 1;
//		}
//
//		template <Int Idx, typename StreamTypes>
//		void stream(PkdVTree<StreamTypes>* sizes, Int idx)
//		{
//			typedef typename PkdVTree<StreamTypes>::Values Values;
//
//			auto size = std::get<Idx - 1>(labels_);
//
//			Values values;
//			values[0] = size;
//
//			sizes->insert(idx, values);
//
//			std::get<Idx>(delta_)[0] = 1;
//			std::get<Idx>(delta_)[1] = size;
//		}
//	};
//
//
//
//
//	struct InsertNodeFn {
//
//		Accumulator& delta_;
//		const LabelsTuple& labels_;
//
//		InsertNodeFn(Accumulator& delta, const LabelsTuple& labels):
//			delta_(delta),
//			labels_(labels)
//		{}
//
//		template <Int Idx, typename SeqTypes>
//		void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Int symbol)
//		{
//			MEMORIA_ASSERT_TRUE(seq != nullptr);
//
//			auto old_indexes = seq->sums();
//
//			seq->insert(idx, symbol);
//
//			auto new_indexes = seq->sums();
//
//			auto indexes = new_indexes - old_indexes;
//
//			std::get<Idx>(delta_) = indexes;
//		}
//
//		template <typename NTypes, bool root, typename... Labels>
//		void treeNode(
//				TreeNode<TreeLeafNode, NTypes, root, true>* node,
//				Int node_idx,
//				Int label_idx,
//				Int symbol
//			)
//		{
//			node->layout(-1);
//			node->template processStream<0>(*this, node_idx, symbol);
//
//			InsertLabelsFn fn(delta_, labels_);
//			node->processAll(fn, label_idx);
//		}
//
//		template <typename NTypes, bool root, typename... Labels>
//		void treeNode(
//				TreeNode<TreeLeafNode, NTypes, root, true>* node,
//				Int node_idx
//		)
//		{
//			node->layout(1);
//			node->template processStream<0>(*this, node_idx, 0);
//		}
//	};
//
//
//
//
//
//
//	bool insertLoudsNode(
//			NodeBaseG& leaf,
//			Int node_idx,
//			Int label_idx,
//			Accumulator& sums,
//			const LabelsTuple& labels
//	)
//	{
//		auto& self = this->self();
//
//		PageUpdateMgr mgr(self);
//
//		mgr.add(leaf);
//
//		try {
//			LeafDispatcher::dispatch(
//					leaf,
//					InsertNodeFn(sums, labels),
//					node_idx,
//					label_idx,
//					1
//			);
//
//			return true;
//		}
//		catch (PackedOOMException& e)
//		{
//			mgr.rollback();
//			return false;
//		}
//	}
//
//
//
//
//
//
//	bool insertLoudsZero(
//			NodeBaseG& leaf,
//			Int node_idx,
//			Accumulator& sums
//	)
//	{
//		auto& self = this->self();
//
//		PageUpdateMgr mgr(self);
//
//		mgr.add(leaf);
//
//		try {
//			LeafDispatcher::dispatch(leaf, InsertNodeFn(sums, LabelsTuple()), node_idx);
//			return true;
//		}
//		catch (PackedOOMException& e)
//		{
//			mgr.rollback();
//			return false;
//		}
//	}
//
//
//	void split(Iterator& iter)
//	{
//		auto& self 	= this->self();
//		auto& leaf 	= iter.leaf();
//		Int& idx	= iter.idx();
//		Int stream 	= iter.stream();
//		Int size 	= iter.leaf_size(stream);
//
//		Int split_idx = size / 2;
//		Int label_idx = iter.label_idx(split_idx);
//
//		auto right = self.splitLeafP(leaf, {split_idx, label_idx, label_idx});
//
//		if (idx > split_idx)
//		{
//			leaf = right;
//			idx -= split_idx;
//		}
//	}
//
//
//	void insertNode(Iterator& iter, const LabelsTuple& labels)
//	{
//		auto& self 	= this->self();
//		auto& leaf 	= iter.leaf();
//		Int& idx	= iter.idx();
////		Int stream 	= iter.stream();
//
//		Int label_idx = iter.label_idx();
//
//		Accumulator sums;
//
//		if (self.insertLoudsNode(leaf, idx, label_idx, sums, labels))
//		{
//			self.updateParent(leaf, sums);
//		}
//		else
//		{
//			self.split(iter);
//
//			label_idx = iter.label_idx();
//
//			MEMORIA_ASSERT_TRUE(self.insertLoudsNode(leaf, idx, label_idx, sums, labels));
//			self.updateParent(leaf, sums);
//		}
//
//		Position sizes(1);
//
//		self.addTotalKeyCount(sizes);
//	}
//
//	void insertZero(Iterator& iter)
//	{
//		auto& self 	= this->self();
//		auto& leaf 	= iter.leaf();
//		Int& idx	= iter.idx();
////		Int stream 	= iter.stream();
//
//		Accumulator sums;
//
//		if (self.insertLoudsZero(leaf, idx, sums))
//		{
//			self.updateParent(leaf, sums);
//		}
//		else
//		{
//			self.split(iter);
//
//			MEMORIA_ASSERT_TRUE(self.insertLoudsZero(leaf, idx, sums));
//			self.updateParent(leaf, sums);
//		}
//
//		self.addTotalKeyCount(Position::create(0, 1));
//	}
//
//	LoudsNode newNodeAt(const LoudsNode& node, const LabelsTuple& labels)
//	{
//		auto& self = this->self();
//
//		Iterator iter = self.findNode(node);
//
//		self.insertNode(iter, labels);
//
//		iter = self.firstChild(iter.node());
//
//		self.insertZero(iter);
//
//		return iter.node();
//	}


MEMORIA_CONTAINER_PART_END

}


#endif
