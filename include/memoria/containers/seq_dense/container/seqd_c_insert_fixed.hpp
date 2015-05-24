
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_FIXED_HPP
#define _MEMORIA_CONTAINERS_SEQD_SEQ_C_INSERT_FIXED_HPP


#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/containers/seq_dense/seqd_walkers.hpp>
#include <memoria/containers/seq_dense/seqd_names.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::seq_dense::CtrInsertFixedName)

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

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    static const Int Streams                                                    = Types::Streams;

    static const Int MAIN_STREAM                                                = Types::MAIN_STREAM;


    //==========================================================================================

    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutLeafNode(NodeBaseG& node, Int size) const
    {
    	LeafDispatcher::dispatch(node, LayoutNodeFn(), Position(size));
    }

    struct InsertBufferIntoLeafFn
    {
    	template <typename NTypes, typename LeafPosition, typename Buffer>
    	void treeNode(LeafNode<NTypes>* node, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    	{
    		node->processAll(*this, pos, start, size, buffer);
    	}

    	template <typename StreamType, typename LeafPosition, typename Buffer>
    	void stream(StreamType* obj, LeafPosition pos, LeafPosition start, LeafPosition size, const Buffer* buffer)
    	{
    		obj->insert(buffer, pos, start, size);
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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::seq_dense::CtrInsertFixedName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
