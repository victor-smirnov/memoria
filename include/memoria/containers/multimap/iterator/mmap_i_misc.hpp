
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MULTIMAP_ITER_MISC_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_ITER_MISC_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::mmap::ItrMiscName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 	= typename Container::Types::CtrSizeT;
    using Key		= typename Container::Types::Key;
    using Value		= typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;

    void insertKey(Key key)
    {
    }

    struct FindGTFn {
    	template <typename PackedStruct, typename T>
    	Int stream(const PackedStruct* obj, Int block, Int start, T value)
    	{
    		return obj->findGTForward(block, start, value).idx();
    	}
    };

    struct SumFn {
    	template <typename PackedStruct>
    	auto stream(const PackedStruct* obj, Int index, Int idx) -> typename PackedStruct::Value
    	{
    		return obj->sum(index, idx);
    	}
    };


    class LocalPos {
    	Int keyIdx_;
    	CtrSizeT dataIdx_;
    public:
    	LocalPos(): keyIdx_(0), dataIdx_(0) {}
    	LocalPos(Int keyIdx, CtrSizeT dataIdx): keyIdx_(keyIdx), dataIdx_(dataIdx) {}
    	Int keyIdx() const {return keyIdx_;}
    	CtrSizeT dataIdx() const {return dataIdx_;}
    };

    struct ToLocalFn {
    	template <typename PackedStruct, typename T>
    	LocalPos stream(const PackedStruct* obj, Int block, Int start, T pos)
    	{
    		auto result = obj->findGTForward(block, start, pos);

    		return LocalPos(result.idx(), result.prefix() - result.idx());
    	}
    };


    LocalPos toLocalPos(Int idx) const
    {
    	auto& self = this->self();
    	return std::get<0>(self.ctr().template _applySubstreamsFn<0, IntList<0>>(self.leaf(), ToLocalFn(), 1, 0, idx));
    }

    LocalPos toLocalPos() const
    {
    	auto& self = this->self();
    	return self.toLocalPos(self.idx());
    }


    Key key() const
    {
    	return self().rawKey() + self().keyPrefix();
    }

    auto rawKey() const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<0>, Int, Int>>::type
    {
    	auto& self = this->self();
    	return std::get<0>(self.ctr().template _readLeafStreamEntry<0, IntList<0>>(self.leaf(), 0, self.toLocalPos().keyIdx()));
    }

    Key keyPrefix() const {
    	auto& self = this->self();
    	auto& cache = self.cache();

    	return bt::Path<0, 0>::get(cache.prefixes())[0] +
    		   bt::Path<0, 0>::get(cache.leaf_prefixes())[0];
    }


    Key prefix() const {
    	return 0;
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mmap::ItrMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
