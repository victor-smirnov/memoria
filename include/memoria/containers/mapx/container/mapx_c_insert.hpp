
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPX_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_MAPX_CTR_INSERT_HPP


#include <memoria/containers/mapx/mapx_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/mapx/mapx_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::mapx::CtrRemoveName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef ValuePair<Accumulator, Value>                                       Element;

    typedef typename Types::Entry                                               MapEntry;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

    CtrSizeT size() const {
    	return self().sizes()[0];
    }

    Iterator Begin() {
    	return self().template _seek<0>(0);
    }

    Iterator End() {
    	auto& self = this->self();
    	return self.template _seek<0>(self.sizes()[0]);
    }

    Iterator begin() {
    	return self().template _seek<0>(0);
    }

    Iterator end() {
    	auto& self = this->self();
    	return self.template _seek<0>(self.sizes()[0]);
    }


    Iterator find(const TargetType<IntList<0>>& k)
    {
    	return self().template _find2GE<IntList<0>>(0, k);
    }


    bool remove(const TargetType<IntList<0>>& k)
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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mapx::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}


#endif
