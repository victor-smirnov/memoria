
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_seqd_ITERATOR_API_HPP
#define _MEMORIA_CONTAINERS_seqd_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterAPIName)

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
		return self().skipFw(1);
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

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int idx) {}

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, Int idx)
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

	struct SymbolFn {
		Int symbol_ = 0;

		template <Int Idx, typename SeqTypes>
		void stream(const PkdFSSeq<SeqTypes>* seq, Int idx)
		{
			MEMORIA_ASSERT_TRUE(seq != nullptr);
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

	BigInt rank(BigInt delta, Int symbol)
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

	BigInt rankFw(BigInt delta, Int symbol)
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

	BigInt rankBw(BigInt delta, Int symbol)
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
    
MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterAPIName)
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
