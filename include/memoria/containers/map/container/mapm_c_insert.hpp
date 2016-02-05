
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

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef ValuePair<BranchNodeEntry, Value>                                       Element;

    typedef typename Types::Entry                                               MapEntry;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT size() const {
    	return self().sizes()[0];
    }

    template <typename K>
    Iterator find(const K& k)
    {
    	return self().template find_max_ge<IntList<0, 1>>(0, k);
    }

    template <typename K>
    bool remove(const K& k)
    {
    	auto iter = find(k);

    	if (iter.key() == k)
    	{
    		iter.remove();
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
