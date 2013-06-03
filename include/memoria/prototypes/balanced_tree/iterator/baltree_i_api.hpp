
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/core/container/macros.hpp>


#include <iostream>


namespace memoria    {

using namespace memoria::balanced_tree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::balanced_tree::IteratorAPIName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Element                                   Element;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Types::Position											Position;


    bool nextLeaf();
    bool nextLeafMs(UBigInt streams);


    bool prevLeaf();

    bool next() {
        return self().nextKey();
    }

    bool prev() {
        return self().prevKey();
    }

    void updateUp(const Accumulator& keys)
    {
    	auto& self = this->self();
    	self.model().updateUp(self.path(), 0, self.key_idx(), keys);
    }


    bool IsFound() {
    	auto& self = this->self();
    	return (!self.isEnd()) && self.isNotEmpty();
    }

    void dumpKeys(ostream& out) const
    {
        Base::dumpKeys(out);
//        out<<"Prefix:  "<<self()->prefixes()<<endl;
    }

    BigInt skipStreamFw(Int stream, BigInt distance);
    BigInt skipStreamBw(Int stream, BigInt distance);
    BigInt skipStream(Int stream, BigInt distance);

    MEMORIA_DECLARE_NODE_FN_RTN(SizeFn, size, Int);

    Int leafSize(Int stream) const
    {
    	return self().leaf_size(stream);
    }

    Int leaf_size(Int stream) const
    {
    	return LeafDispatcher::dispatchConstRtn(self().path().leaf().node(), SizeFn(), stream);
    }

    Int leaf_size() const
    {
    	return LeafDispatcher::dispatchConstRtn(self().path().leaf().node(), SizeFn(), self().stream());
    }

    MEMORIA_DECLARE_NODE_FN_RTN(SizesFn, sizes, Int);

    Position leaf_sizes() const {
    	return LeafDispatcher::dispatchConstRtn(self().path().leaf().node(), SizesFn());
    }

    bool has_no_data() const
    {
    	return leaf_sizes().eqAll(0);
    }

    bool is_leaf_empty() const
    {
    	return self().model().isNodeEmpty(self().path().leaf().node());
    }

    Int leaf_capacity(Int stream) const
    {
    	auto& self = this->self();
    	return self.leaf_capacity(Position(), stream);
    }

    Int leaf_capacity(const Position& reserved, Int stream) const
    {
    	auto& self = this->self();
    	auto& ctr = self.model();

    	return ctr.getStreamCapacity(self.leaf(), reserved, stream);
    }

    template <typename Walker>
    bool findNextLeaf(Walker&& walker);

    template <typename Walker>
    bool findPrevLeaf(Walker&& walker);

    void createEmptyLeaf() {

    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::balanced_tree::IteratorAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
BigInt M_TYPE::skipStreamFw(Int stream, BigInt amount)
{
	typedef typename Types::template SkipForwardWalker<Types> Walker;

	auto& self = this->self();

	Walker walker(stream, 0, amount);

	walker.prepare(self);

	Int idx = self.model().findFw(self.path(), stream, self.key_idx(), walker);

	return walker.finish(self, idx);
}

M_PARAMS
BigInt M_TYPE::skipStreamBw(Int stream, BigInt amount)
{
	typedef typename Types::template SkipBackwardWalker<Types> Walker;

	auto& self = this->self();

	Walker walker(stream, 0, amount);

	walker.prepare(self);

	Int idx = self.model().findBw(self.path(), stream, self.key_idx(), walker);

	return walker.finish(self, idx);
}




M_PARAMS
BigInt M_TYPE::skipStream(Int stream, BigInt amount)
{
    auto& self = this->self();

	if (amount > 0)
    {
        return self.skipStreamFw(stream, amount);
    }
    else if (amount < 0) {
        return self.skipStreamBw(stream, -amount);
    }
    else {
    	return 0;
    }
}

M_PARAMS
bool M_TYPE::nextLeafMs(UBigInt streams)
{
	typedef typename Types::template NextLeafMutistreamWalker<Types> Walker;

	auto& self = this->self();

	Walker walker(streams);

	return self.findNextLeaf(walker);
}


M_PARAMS
bool M_TYPE::nextLeaf()
{
	typedef typename Types::template NextLeafWalker<Types> Walker;
	Walker walker(self().stream(), 0);

	return self().findNextLeaf(walker);
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
	typedef typename Types::template PrevLeafWalker<Types> Walker;

	auto& self = this->self();
	Int stream = self.stream();

	Walker walker(stream, 0);

	return self.findPrevLeaf(walker);
}



M_PARAMS
template <typename Walker>
bool M_TYPE::findNextLeaf(Walker&& walker)
{
	auto& self = this->self();

	TreePath& 	path 	= self.path();
	Int 		stream 	= self.stream();

	if (path.getSize() > 1)
	{
		walker.prepare(self);

		Int idx = self.model().findFw(path, stream, self.leaf().parent_idx() + 1, walker, 1);

		Int size = self.model().getNodeSize(path[1].node(), stream);

		MEMORIA_ASSERT_TRUE(size > 0);

		Int child_idx;

		if (idx < size)
		{
			child_idx = idx;
		}
		else {
			child_idx = size - 1;
		}

		// Step down the tree
		path[0].node() 			= self.model().getChild(path[1].node(), child_idx, Allocator::READ);
		path[0].parent_idx()	= child_idx;

		walker.finish(self, idx < size);

		self.idx() = 0;

		return idx < size;
	}
	else {
		return false;
	}
}



M_PARAMS
template <typename Walker>
bool M_TYPE::findPrevLeaf(Walker&& walker)
{
	auto& self = this->self();

	TreePath& 	path 	= self.path();
	Int 		stream 	= self.stream();

	if (path.getSize() > 1)
	{
		walker.prepare(self);

		Int idx = self.model().findBw(path, stream, self.leaf().parent_idx() - 1, walker, 1);

		Int size = self.model().getNodeSize(path[1].node(), stream);

		MEMORIA_ASSERT_TRUE(size > 0);

		Int child_idx;

		if (idx >= 0)
		{
			child_idx = idx;
		}
		else {
			child_idx = 0;
		}

		// Step down the tree
		path[0].node() 			= self.model().getChild(path[1].node(), child_idx, Allocator::READ);
		path[0].parent_idx()	= child_idx;

		walker.finish(self, idx >= 0);

		self.idx() = idx >= 0 ? self.leafSize(stream) - 1 : -1;

		return idx >= 0;
	}
	else {
		return false;
	}
}


#undef M_TYPE
#undef M_PARAMS


}


#endif
