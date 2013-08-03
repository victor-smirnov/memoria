
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterMiscName)

	typedef Ctr<typename Types::CtrTypes>                      					Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     		Value;
	typedef typename Container::Key                                       		Key;
	typedef typename Container::Element                                   		Element;
	typedef typename Container::Accumulator                               		Accumulator;

//	typedef typename Container::DataSource                                		DataSource;
//	typedef typename Container::DataTarget                                		DataTarget;
	typedef typename Container::LeafDispatcher                                	LeafDispatcher;
	typedef typename Container::Position										Position;


	Int size() const
	{
		return self().leafSize(0);
	}

	struct SymbolFn {
		Int symbol_ = 0;

		template <Int Idx, typename SeqTypes>
		void stream(const PkdFSSeq<SeqTypes>* seq, Int idx)
		{
			MEMORIA_ASSERT_TRUE(seq != nullptr);

			if (idx >= seq->size()) {
				int a = 0; a++;
			}

			MEMORIA_ASSERT(idx, <, seq->size());

			symbol_ = seq->symbol(idx);
		}


		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int idx)
		{
			node->process(0, *this, idx);
		}
	};

	Int symbol() const
	{
		auto& self 	= this->self();

		SymbolFn fn;

		Int idx = self.idx();

		LeafDispatcher::dispatchConst(self.leaf(), fn, idx);

		return fn.symbol_;
	}

	void insert(Int symbol)
	{
		auto& self 	= this->self();
		auto& ctr 	= self.ctr();

		ctr.insert(self, symbol);
	}

	void remove(Int symbol)
	{

	}
MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS


}



#endif
