
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPM_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_MAPM_CTR_INSERT_HPP


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::map::CtrInsertMaxName)

    using typename Base::Types;

    using typename Base::NodeBaseG;
    using typename Base::IteratorPtr;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::PageUpdateMgr;

    using Key = typename Types::Key;
    using Value = typename Types::Value;



    static const Int Streams = Types::Streams;

    using Element 	= ValuePair<BranchNodeEntry, Value>;
    using MapEntry 	= typename Types::Entry;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT size() const {
    	return self().sizes()[0];
    }

    template <typename K>
    IteratorPtr find(const K& k)
    {
    	return self().template find_max_ge<IntList<0, 1>>(0, k);
    }

    template <typename K>
    bool remove(const K& k)
    {
    	auto iter = find(k);

    	if (iter->key() == k)
    	{
    		iter->remove();
    		return true;
    	}
    	else {
    		return false;
    	}
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}


#endif
