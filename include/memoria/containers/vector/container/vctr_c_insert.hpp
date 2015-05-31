
// Copyright Victor Smirnov 2013+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_vctr_C_INSERT_HPP
#define _MEMORIA_CONTAINER_vctr_C_INSERT_HPP


#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::CtrInsertName)

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


    typedef typename Types::CtrSizeT                                            CtrSizeT;
    typedef typename Types::Value                                               Value;

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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


#undef M_PARAMS
#undef M_TYPE

}


#endif
