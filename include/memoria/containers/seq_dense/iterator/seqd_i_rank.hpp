
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_RANK_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_RANK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterRankName)

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


	BigInt rank(BigInt delta, Int symbol);
	BigInt rankFw(BigInt delta, Int symbol);
	BigInt rankBw(BigInt delta, Int symbol);

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterRankName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::rank(BigInt delta, Int symbol)
{
	auto& self 	= this->self();

	if (delta > 0)
	{
		return self.rankFw(delta, symbol);
	}
	else if (delta < 0)
	{
		return self.rankBw(-delta, symbol);
	}
	else {
		return 0;
	}
}

M_PARAMS
BigInt M_TYPE::rankFw(BigInt delta, Int symbol)
{
	auto& self 	= this->self();
	auto& ctr 	= self.ctr();
	Int stream  = self.stream();

	MEMORIA_ASSERT(delta, >=, 0);

	typename Types::template RankFWWalker<Types> walker(stream, symbol, delta);

	walker.prepare(self);

	Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

	return walker.finish(self, idx);
}

M_PARAMS
BigInt M_TYPE::rankBw(BigInt delta, Int symbol)
{
	auto& self 	= this->self();
	auto& ctr 	= self.ctr();
	Int stream  = self.stream();

	MEMORIA_ASSERT(delta, >=, 0);

	typename Types::template RankBWWalker<Types> walker(stream, symbol, delta);

	walker.prepare(self);

	Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

	return walker.finish(self, idx);
}




#undef M_TYPE
#undef M_PARAMS


}



#endif
