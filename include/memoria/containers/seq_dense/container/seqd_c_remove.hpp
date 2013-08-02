
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_REMOVE_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_REMOVE_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrRemoveName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	typedef typename Types::PageUpdateMgr										PageUpdateMgr;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	static const Int MAIN_STREAM												= Types::MAIN_STREAM;

	struct RemoveFromLeafFn {

		template <Int Idx, typename SeqTypes>
		void stream(PkdFSSeq<SeqTypes>* seq, Int idx, Accumulator* delta)
		{
			MEMORIA_ASSERT_TRUE(seq != nullptr);

			typedef PkdFSSeq<SeqTypes> 	Seq;
			typedef typename Seq::Values 				Values;

			Int sym = seq->symbol(idx);

			seq->remove(idx, idx + 1);

			Values indexes;

			indexes[0]  	 = -1;
			indexes[sym + 1] = -1;

			std::get<Idx>(*delta) = indexes;
		}


		template <typename NTypes, bool root>
		void treeNode(TreeNode<TreeLeafNode, NTypes, root, true>* node, Int stream, Int idx, Accumulator* delta)
		{
			node->layout(1);
			node->process(stream, *this, idx, delta);
		}
	};



	void removeFromLeaf(NodeBaseG& leaf, Int idx, Accumulator& indexes)
	{
		LeafDispatcher::dispatch(leaf, RemoveFromLeafFn(), 0, idx, &indexes);
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
		Int stream 	= iter.stream();

		Accumulator sums;

		self.removeFromLeaf(leaf, idx, sums);

		self.updateParent(leaf, sums);

		self.addTotalKeyCount(Position::create(stream, -1));

		self.mergeWithSiblings(leaf);
	}




MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif