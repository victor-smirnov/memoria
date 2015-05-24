
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_REMOVE_HPP


#include <memoria/containers/mapx/mapx_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/mapx/mapx_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::RemoveName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    template <Int Stream>
    void removeStreamEntry(Iterator& iter)
    {
    	auto& self      = this->self();

    	auto result = self.template tryRemoveStreamEntry<Stream>(iter);

    	if (!std::get<0>(result))
    	{
    		iter.split();

    		result = self.template tryRemoveStreamEntry<Stream>(iter);

    		if (!std::get<0>(result))
    		{
    			throw Exception(MA_SRC, "Second removal attempt failed");
    		}

    		self.updateParent(iter.leaf(), std::get<1>(result));
    	}
    	else {
    		self.updateParent(iter.leaf(), std::get<1>(result));

    		auto next = self.getNextNodeP(iter.leaf());

    		if (next.isSet())
    		{
    			self.mergeLeafNodes(iter.leaf(), next, [](const Position&){});
    		}

    		auto prev = self.getPrevNodeP(iter.leaf());

    		if (prev.isSet())
    		{
    			self.mergeLeafNodes(prev, iter.leaf(), [&iter, &prev](const Position& sizes){
    				iter.idx() += sizes[0];
    				iter.leaf() = prev;
    			});
    		}
    	}

    	self.addTotalKeyCount(self.getStreamSizes(std::get<1>(result)));
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::RemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
