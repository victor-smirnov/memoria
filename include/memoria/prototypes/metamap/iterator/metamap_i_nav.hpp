
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_NAV_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_NAV_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;

    typedef typename Base::Container::Types::CtrSizeT                           CtrSizeT;

    bool operator++()
    {
        self().skipFw(1);
        return !self().isEnd();
    }

    bool operator--()
    {
        self().skipBw(1);
        return !self().isBegin();
    }

    bool operator++(int)
    {
        self().skipFw(1);
        return !self().isEnd();
    }

    bool operator--(int)
    {
    	self().skipBw(1);

        return !self().isBegin();
    }

    CtrSizeT operator+=(BigInt size)
    {
        return self().skipFw(size);
    }

    CtrSizeT operator-=(BigInt size)
    {
        return self().skipBw(size);
    }

    CtrSizeT pos() const
    {
    	return std::get<0>(self().cache().prefixes())[0];
    }

    bool nextLeaf();
    bool prevLeaf();


    CtrSizeT skipFw(BigInt amount);
    CtrSizeT skipBw(BigInt amount);
    CtrSizeT skip(BigInt amount);

    CtrSizeT skipStreamFw(Int stream, BigInt distance)
    {
        return skipFw(distance);
    }

    CtrSizeT skipStreamBw(Int stream, BigInt distance)
    {
        return skipBw(distance);
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::metamap::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


//FIXME: Should nextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::nextLeaf()
{
    auto& self = this->self();

    auto next = self.ctr().getNextNodeP(self.leaf());

    if (next)
    {
        self.leaf() = next;
        self.idx()  = 0;
        return true;
    }

    return false;
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
    auto& self = this->self();

    auto prev = self.ctr().getPrevNodeP(self.leaf());

    if (prev)
    {
        self.leaf() = prev;
        self.idx()  = self.leafSize(self.stream()) - 1;

        return true;
    }

    return false;
}




M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipFw(BigInt amount)
{
	return self().template _findFw<Types::template SkipForwardWalker>(0, amount);
}


M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skipBw(BigInt amount)
{
	return self().template _findBw<Types::template SkipBackwardWalker>(0, amount);
}


M_PARAMS
typename M_TYPE::CtrSizeT M_TYPE::skip(BigInt amount)
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




}

#undef M_TYPE
#undef M_PARAMS


#endif
