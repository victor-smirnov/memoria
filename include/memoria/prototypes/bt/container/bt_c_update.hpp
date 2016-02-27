
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_UPDATE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_UPDATE_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::UpdateName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    static const Int Streams                                                    = Types::Streams;


    using CtrSizeT = typename Types::CtrSizeT;

    template <Int Stream>
    using StreamInputTuple = typename Types::template StreamInputTuple<Stream>;

//    template <Int Stream, typename SubstreamsList, typename... TupleTypes>
//    void update_stream_entry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
//    {
//    	auto& self = this->self();
//
//    	auto result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, entry);
//
//    	if (!std::get<0>(result))
//    	{
//    		iter.split();
//
//    		result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, entry);
//
//    		if (!std::get<0>(result))
//    		{
//    			throw Exception(MA_SRC, "Second insertion attempt failed");
//    		}
//    	}
//
//    	auto max = self.max(iter.leaf());
//
//    	self.update_parent(iter.leaf(), max);
//    }


    template <Int Stream, typename SubstreamsList, typename Buffer>
    void update_stream_entry(Iterator& iter, const Buffer& entry)
    {
    	auto& self = this->self();

    	auto result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, entry);

    	if (!std::get<0>(result))
    	{
    		iter.split();

    		result = self.template try_update_stream_entry<Stream, SubstreamsList>(iter, entry);

    		if (!std::get<0>(result))
    		{
    			throw Exception(MA_SRC, "Second insertion attempt failed");
    		}
    	}

    	auto max = self.max(iter.leaf());

    	self.update_parent(iter.leaf(), max);
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::UpdateName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
