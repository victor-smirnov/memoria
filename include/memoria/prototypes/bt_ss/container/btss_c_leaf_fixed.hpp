
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_FIXED_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_FIXED_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/layouts/bt_input_buffer.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btss::LeafFixedName)

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

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    struct InsertBufferIntoLeafFn
    {
    	template <typename NTypes, typename LeafPosition, typename Buffer>
    	void treeNode(LeafNode<NTypes>* node, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    	{
    		node->processAll(*this, pos, start, size, buffer);
    	}

    	template <Int ListIdx, typename StreamType, typename LeafPosition, typename Buffer>
    	void stream(StreamType* obj, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    	{
    		obj->insert(pos, start, size, buffer->template geti<ListIdx>());
    	}
    };


    template <typename LeafPosition>
    using InsertBufferResult = typename Base::template InsertBufferResult<LeafPosition>;

    template <typename LeafPosition, typename Buffer>
    InsertBufferResult<LeafPosition> insertBufferIntoLeaf(NodeBaseG& leaf, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    {
    	auto& self = this->self();

    	Int sizes = size - start;

    	Int capacity = self.getLeafNodeCapacity(leaf, 0);

    	Int to_insert = capacity >= sizes ? sizes : capacity;

    	LeafDispatcher::dispatch(leaf, InsertBufferIntoLeafFn(), pos, start, to_insert, buffer);

    	return InsertBufferResult<LeafPosition>(to_insert, capacity > to_insert);
    }


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::btss::LeafFixedName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS

}



#endif
