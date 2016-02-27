
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPM_ITER_NAV_MAX_HPP
#define _MEMORIA_CONTAINERS_MAPM_ITER_NAV_MAX_HPP

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/types/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::map::ItrNavMaxName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::BranchNodeEntry                               BranchNodeEntry;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;



    void insert(const Key& key, const Value& value)
    {
    	auto& self = this->self();

    	self.ctr().insert_entry(
    			self,
				map::KeyValueEntry<Key, Value, CtrSizeT>(key, value)
    	);


    	self.skipFw(1);
    }

//    template <typename InputIterator>
//    void insert(InputIterator&&, InputIterator&&) {}
//
//    template <typename Provider>
//    void insert(Provider&&) {}

    template <typename EntriesProvider>
    auto bulk_insert(EntriesProvider&& provider, Int ib_capacity = 10000)
    {
    	using Provider = map::MapEntryInputProvider<Container, EntriesProvider>;

    	auto bulk = std::make_unique<Provider>(self().ctr(), provider, ib_capacity);

    	return Base::insert(*bulk.get());
    }

    void remove()
    {
    	auto& self = this->self();

    	self.ctr().removeEntry(self);

    	if (self.isEnd())
    	{
    		self.skipFw(0);
    	}
    }



    auto findFwGT(Int index, Key key)
    {
    	return self().template find_fw_gt<IntList<1>>(index, key);
    }

    auto findFwGE(Int index, Key key)
    {
    	return self().template find_fw_ge<IntList<1>>(index, key);
    }

    auto findBwGT(Int index, Key key)
    {
    	return self().template find_bw_gt<IntList<1>>(index, key);
    }

    auto findBwGE(Int index, Key key)
    {
    	return self().template find_bw_ge<IntList<1>>(index, key);
    }

    auto key() const -> Key
    {
    	return std::get<0>(self().ctr().template read_leaf_entry<IntList<1>>(self().leaf(), self().idx(), 0));
    }

    auto value() const
    {
    	return std::get<0>(self().ctr().template read_leaf_entry<IntList<2>>(self().leaf(), self().idx()));
    }

    void assign(const Value& v)
    {
    	self().ctr().template update_entry<IntList<2>>(self(), map::ValueBuffer<Value>(v));
    }

    bool is_found(const Key& k) const
    {
    	auto& self = this->self();
    	return (!self.is_end()) && self.key() == k;
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::map::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
