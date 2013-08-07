
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_LOUDS_C_REMOVE_HPP
#define MEMORIA_CONTAINERS_LOUDS_LOUDS_C_REMOVE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/louds/louds_names.hpp>
#include <memoria/containers/louds/louds_tools.hpp>
#include <memoria/containers/seq_dense/seqd_walkers.hpp>


#include <memoria/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrRemoveName)

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


	struct RemoveFromLeafFn {
		Accumulator& delta_;
		Position sizes_;

		Int label_idx_;

		RemoveFromLeafFn(Accumulator& delta): delta_(delta) {}

		template <Int Idx, typename SeqTypes>
		void stream(PkdFSSeq<SeqTypes>* seq, Int idx)
		{
			MEMORIA_ASSERT_TRUE(seq != nullptr);

			typedef PkdFSSeq<SeqTypes> 	Seq;
			typedef typename Seq::Values 				Values;

			Int sym = seq->symbol(idx);

			if (sym) {
				label_idx_ = seq->rank(idx, 1);
			}
			else {
				label_idx_ = -1;
			}

			seq->remove(idx, idx + 1);

			Values indexes;

			indexes[0]  	 = -1;
			indexes[sym + 1] = -1;

			std::get<Idx>(delta_) = indexes;

			sizes_[Idx] = -1;
		}

		template <Int Idx, typename StreamTypes>
		void stream(PackedFSEArray<StreamTypes>* labels, Int idx)
		{
			if (label_idx_ >= 0)
			{
				labels->remove(label_idx_, label_idx_ + 1);
				std::get<Idx>(delta_)[0] = -1;

				sizes_[Idx] = -1;
			}
		}

		template <Int Idx, typename StreamTypes>
		void stream(PkdVTree<StreamTypes>* sizes, Int idx)
		{
			if (label_idx_ >= 0)
			{
				typename PkdVTree<StreamTypes>::Value size = sizes->value(0, label_idx_);

				sizes->remove(label_idx_, label_idx_ + 1);

				std::get<Idx>(delta_)[0] = -1;
				std::get<Idx>(delta_)[1] = -size;

				sizes_[Idx] = -1;
			}
		}



		template <typename NTypes, bool root>
		void treeNode(TreeNode<TreeLeafNode, NTypes, root, true>* node, Int idx)
		{
			node->processAll(*this, idx);
		}
	};

	Position removeFromLeaf(NodeBaseG& leaf, Int idx, Accumulator& indexes)
	{
		RemoveFromLeafFn fn(indexes);
		LeafDispatcher::dispatch(leaf, fn, idx);

		return fn.sizes_;
	}

	void remove(BigInt idx)
	{
		auto& self 	= this->self();
		auto iter 	= self.seek(idx);

		self.remove(iter);
	}

	void remove(Iterator& iter)
	{
		auto& self 	= this->self();
		auto& leaf 	= iter.leaf();
		Int& idx	= iter.idx();

		Accumulator sums;

		Position sizes = removeFromLeaf(leaf, idx, sums);

		self.updateParent(leaf, sums);

		self.addTotalKeyCount(sizes);

		self.mergeWithSiblings(leaf);
	}


	void removeLeaf(const LoudsNode& node)
	{
		auto& self = this->self();

		Iterator iter = self.findNode(node);

		MEMORIA_ASSERT_TRUE(iter.symbol() == 1);

		iter.firstChild();

		MEMORIA_ASSERT_TRUE(iter.symbol() == 0);

		iter.remove();

		iter = self.findNode(node);
		iter.remove();
	}

MEMORIA_CONTAINER_PART_END

}


#endif
