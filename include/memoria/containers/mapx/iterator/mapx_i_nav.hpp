
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP
#define _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::mapx::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    bool operator++() {
        return self().nextKey();
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

    Key getRawKey() const
    {
        auto& self = this->self();
        return self.raw_key();
    }

    Accumulator getRawKeys() const
    {
        auto& self = this->self();

        Accumulator accum;

        std::get<0>(accum)[0] = self.raw_key();

        return accum;
    }


    CtrSizeT skipFw(CtrSizeT amount) {
    	return self().template _skipFw<0>(amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
    	return self().template _skipBw<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
    	return self().template _skip<0>(amount);
    }

    BigInt skipStreamFw(Int stream, BigInt distance) {
        return skipFw(distance);
    }

    BigInt skipStreamBw(Int stream, BigInt distance) {
        return skipBw(distance);
    }

    void split()
    {
    	auto& self = this->self();

    	NodeBaseG& leaf = self.leaf();
    	Int& idx        = self.idx();

    	Int size        = self.leaf_size(0);
    	Int split_idx   = size/2;

    	auto right = self.ctr().splitLeafP(leaf, Position::create(0, split_idx));

    	if (idx > split_idx)
    	{
    		leaf = right;
    		idx -= split_idx;

//    		self.updatePrefix();
    	}
    }

    void insert(BigInt key, BigInt value)
    {
    	auto& self = this->self();

    	self.ctr().template insertStreamEntry<0>(self, std::make_tuple(core::StaticVector<BigInt, 1>({key}), value));
    }

    void remove() {
    	auto& self = this->self();
    	self.ctr().template removeStreamEntry<0>(self);
    }



    auto findFwGT(Int index, BigInt key) ->
    ItrFindFwGTRtnType<Base, IntList<0>, Int, BigInt>
    {
    	return self().template _findFwGT<IntList<0>>(index, key);
    }

    auto findFwGE(Int index, BigInt key) ->
    ItrFindFwGERtnType<Base, IntList<0>, Int, BigInt>
    {
    	return self().template _findFwGE<IntList<0>>(index, key);
    }

    auto findBwGT(Int index, BigInt key) ->
    ItrFindBwGTRtnType<Base, IntList<0>, Int, BigInt>
    {
    	return self().template _findBwGT<IntList<0>>(index, key);
    }

    auto findBwGE(Int index, BigInt key) ->
    ItrFindBwGERtnType<Base, IntList<0>, Int, BigInt>
    {
    	return self().template _findBwGE<IntList<0>>(index, key);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mapx::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
