
// Copyright Victor Smirnov 2014+.
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
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;


    void insert(Key key, Value value)
    {
    	auto& self = this->self();

    	auto delta = key - self.prefix();

    	self.ctr().insertEntry(
    			self,
    			InputTupleAdapter<0>::convert(core::StaticVector<Key, 1>({delta}), value)
    	);

    	self.skipFw(1);

    	if (!self.isEnd())
    	{
    		auto k = self.raw_key();

    		MEMORIA_ASSERT_TRUE((k - StaticVector<BigInt, 1>(delta))[0] >= 0);

    		self.ctr().template updateEntry<IntList<0>>(self, std::make_tuple(k - StaticVector<BigInt, 1>(delta)));
    	}
    }



    void remove() {
    	auto& self = this->self();

    	auto k = self.raw_key();

    	self.ctr().removeEntry(self);

    	if (self.isEnd())
    	{
    		self.skipFw(0);
    	}

    	if (!self.isEnd()) {

    		auto kk = self.raw_key();

    		self.ctr().template updateEntry<IntList<0>>(self, std::make_tuple(k + kk));
    	}
    }



    auto findFwGT(Int index, Key key)
    {
    	return self().template _findFwGT<IntList<0>>(index, key);
    }

    auto findFwGE(Int index, Key key)
    {
    	return self().template _findFwGE<IntList<0>>(index, key);
    }

    auto findBwGT(Int index, Key key)
    {
    	return self().template _findBwGT<IntList<0>>(index, key);
    }

    auto findBwGE(Int index, Key key)
    {
    	return self().template _findBwGE<IntList<0>>(index, key);
    }

    Key prefix() const
    {
    	auto& self = this->self();
    	auto& cache = self.cache();

    	return bt::Path<0, 0>::get(cache.prefixes())[0] +
    		   bt::Path<0, 0>::get(cache.leaf_prefixes())[0];
    }


    template <typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafEntryRtnType<SubstreamsIdxList, Args...>;

    auto raw_key() const
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<IntList<0>>(self().leaf(), self().idx()));
    }

    auto key() const -> Key
    {
    	return self().raw_key(0) + self().prefix();
    }

    auto raw_key(Int index) const
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<IntList<0>>(self().leaf(), self().idx(), index));
    }

    auto value() const
    {
    	return std::get<0>(self().ctr().template _readLeafEntry<IntList<1>>(self().leaf(), self().idx()));
    }


    template <typename TValue>
    void setValue(TValue&& v)
    {
    	self().ctr().template updateEntry<IntList<1>>(self(), std::make_tuple(v));
    }

    bool isFound(Key k) const
    {
    	auto& self = this->self();

    	return (!self.isEnd()) && self.key() == k;
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mapx::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
