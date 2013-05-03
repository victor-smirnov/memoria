
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

    void dumpKeys(ostream& out)
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
    	return LeafDispatcher::dispatchConstRtn(self().path().leaf().node(), SizeFn(), stream);
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

	walker.prepare(self);

	Int idx = self.model().findFw(self.path(), self.stream(), self.key_idx(), walker);

	return walker.finish(self, idx);
}


M_PARAMS
bool M_TYPE::nextLeaf()
{
	typedef typename Types::template NextLeafWalker<Types> Walker;

	auto& self = this->self();
	Int stream = self.stream();

	Walker walker(stream, 0);

	walker.prepare(self);

	Int idx = self.model().findFw(self.path(), stream, self.key_idx(), walker);

	return walker.finish(self, idx);
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
	typedef typename Types::template PrevLeafWalker<Types> Walker;

	auto& self = this->self();
	Int stream = self.stream();

	Walker walker(stream, 0);

	walker.prepare(self);

	Int idx = self.model().findBw(self.path(), stream, self.key_idx(), walker);

	return walker.finish(self, idx);
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
