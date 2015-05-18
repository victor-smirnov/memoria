
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

    template <Int Stream>
    using StreamInputTuple = typename Container::Types::template StreamInputTuple<Stream>;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;


    bool operator++() {
        return self().skipFw(1) == 1;
    }

    bool operator--() {
        return self().skipBw(1) == 1;
    }

    bool operator++(int) {
        return self().skipFw(1) == 1;
    }

    bool operator--(int) {
        return self().skipBw(1) == 1;
    }

    BigInt operator+=(BigInt size)
    {
        return self().skipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
        return self().skipBw(size);
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

    		self.refreshCache();
    	}
    }

    void insert(BigInt key, BigInt value)
    {
    	auto& self = this->self();

    	auto delta = key - self.prefix();

    	self.ctr().template insertStreamEntry<0>(
    			self,
    			InputTupleAdapter<0>::convert(core::StaticVector<BigInt, 1>({delta}), value)
    	);

    	if (!self.isEnd())
    	{
    		auto k = self.raw_key();

    		MEMORIA_ASSERT_TRUE((k - StaticVector<BigInt, 1>(delta))[0] >= 0);

    		self.ctr().template updateStreamEntry<0, IntList<0>>(self, std::make_tuple(k - StaticVector<BigInt, 1>(delta)));
    	}
    }



    void remove() {
    	auto& self = this->self();

    	auto k = self.raw_key();

    	self.ctr().template removeStreamEntry<0>(self);

    	if (self.isEnd())
    	{
    		self.skipFw(0);
    	}

    	if (!self.isEnd()) {

    		auto kk = self.raw_key();

    		self.ctr().template updateStreamEntry<0, IntList<0>>(self, std::make_tuple(k + kk));
    	}
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

    Key prefix() const {
    	return std::get<0>(std::get<0>(self().cache().prefixes()))[0] + std::get<0>(std::get<0>(self().cache().leaf_prefixes()))[0];
    }


    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafEntryRtnType<Stream, SubstreamsIdxList, Args...>;

    auto raw_key() const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<0>, Int>>::type
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<0, IntList<0>>(self().leaf(), self().idx()));
    }

    auto key() const -> Key
    {
    	return self().raw_key(0) + self().prefix();
    }

    auto raw_key(Int index) const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<0>, Int, Int>>::type
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<0, IntList<0>>(self().leaf(), self().idx(), index));
    }

    auto value() const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<1>, Int>>::type
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<0, IntList<1>>(self().leaf(), self().idx()));
    }


    template <typename TValue>
    void set_value1(TValue&& v)
    {
    	self().ctr().template updateStreamEntry<0, IntList<1>>(self(), std::make_tuple(v));
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mapx::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
