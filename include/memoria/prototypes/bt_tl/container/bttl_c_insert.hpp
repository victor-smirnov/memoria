
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_INSERT_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_INSERT_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::InsertName)

    using Types 			= typename Base::Types;

    using NodeBaseG 		= typename Types::NodeBaseG;
    using Iterator  		= typename Base::Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    using Key 				= typename Types::Key;
    using Value 			= typename Types::Value;
    using CtrSizeT			= typename Types::CtrSizeT;
    using CtrSizesT			= typename Types::Position;

    using BranchNodeEntry 		= typename Types::BranchNodeEntry;


    static const Int Streams = Types::Streams;

    using PageUpdateMgt 	= typename Types::PageUpdateMgr;

//    template <typename Provider>
//    CtrSizesT _insert(Iterator& iter, Provider&& provider, const CtrSizesT& buffer = CtrSizesT(2000))
//    {
//    	auto& self = this->self();
//
//    	bttl::StreamingCtrInputProvider2<MyType, Provider> streamingProvider(self, provider, iter.stream(), buffer.sum());
//
//    	auto pos = iter.local_stream_posrank_();
//
//    	streamingProvider.prepare(iter, pos);
//
//    	self.insert_provided_data(iter.leaf(), pos, streamingProvider);
//
//    	return streamingProvider.totals();
//    }

    template <typename Provider>
    CtrSizesT _insert(Iterator& iter, Provider&& provider, const Int total_capacity = 2000)
    {
    	auto& self = this->self();

    	bttl::StreamingCtrInputProvider<MyType, Provider> streamingProvider(self, provider, iter.stream(), total_capacity);

    	auto pos = iter.local_stream_posrank_();

    	streamingProvider.prepare(iter, pos);

    	self.insert_provided_data(iter.leaf(), pos, streamingProvider);

    	return streamingProvider.totals();
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::InsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
