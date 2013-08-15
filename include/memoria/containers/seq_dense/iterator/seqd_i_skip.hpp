
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_SKIP_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_SKIP_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterSkipName)

	typedef Ctr<typename Types::CtrTypes>                      					Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     		Value;
	typedef typename Container::Key                                       		Key;
	typedef typename Container::Element                                   		Element;
	typedef typename Container::Accumulator                               		Accumulator;

	typedef typename Container::LeafDispatcher                                	LeafDispatcher;
	typedef typename Container::Position										Position;

	bool operator++() {
		return self().skipFw(1);
	}

	bool operator--() {
		return self().skipBw(1);
	}

	bool operator++(int) {
		return self().skipFw(1);
	}

	bool operator--(int) {
		return self().skipBw(1);
	}

	BigInt operator+=(BigInt size)
	{
		return self().skipFw(size);
	}

	BigInt operator-=(BigInt size)
	{
		return self().skipBw(size);
	}

	bool isEof() const {
		return self().idx() >= self().size();
	}

	bool isBof() const {
		return self().idx() < 0;
	}

	Int size() const
	{
		return self().leafSize(0);
	}

	BigInt skipFw(BigInt amount);
	BigInt skipBw(BigInt amount);
	BigInt skip(BigInt amount);


	struct PosFn {
		BigInt prefix_ = 0;

		template <typename NodeTypes>
		void treeNode(const LeafNode<NodeTypes>* node, Int idx) {}

		template <typename NodeTypes>
		void treeNode(const BranchNode<NodeTypes>* node, Int idx)
		{
			node->sum(0, 0, 0, idx, prefix_);
		}
	};


	BigInt pos() const
	{
		auto& self = this->self();

		PosFn fn;

		self.ctr().walkUp(self.leaf(), self.idx(), fn);

		return fn.prefix_ + self.idx();
	}

//	BigInt cpos() const
//	{
//		auto& self = this->self();
//		return self.cache().pos();
//	}

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterSkipName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::skip(BigInt amount)
{
    auto& self = this->self();

	if (amount > 0)
    {
        return self.skipFw(amount);
    }
    else if (amount < 0) {
        return self.skipBw(-amount);
    }
    else {
    	return 0;
    }
}


M_PARAMS
BigInt M_TYPE::skipFw(BigInt amount)
{
	return self().template _findFw<Types::template SkipForwardWalker>(0, amount);
}

M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
	return self().template _findBw<Types::template SkipBackwardWalker>(0, amount);
}




#undef M_TYPE
#undef M_PARAMS


}



#endif
