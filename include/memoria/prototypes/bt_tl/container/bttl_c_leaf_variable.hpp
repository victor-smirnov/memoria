
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_LEAF_VARIABLE_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_LEAF_VARIABLE_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::LeafVariableName)

    using Types 			= typename Base::Types;

    using NodeBaseG 		= typename Types::NodeBaseG;
    using Iterator  		= typename Base::Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    using Key 				= typename Types::Key;
    using Value 			= typename Types::Value;
    using CtrSizeT			= typename Types::CtrSizeT;

    using Accumulator 		= typename Types::Accumulator;
    using Position 			= typename Types::Position;

    static const Int Streams = Types::Streams;

    using PageUpdateMgt 	= typename Types::PageUpdateMgr;

    using InsertDataResult = typename Base::InsertDataResult;

    template <typename Provider>
    InsertDataResult insertData(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
    	auto& self = this->self();

    	provider.prepare(leaf, pos);

    	auto last_pos = self.insertDataIntoLeaf(leaf, pos, provider);

    	if (provider.hasData())
    	{
    		// has to be defined in subclasses
    		if (!self.isAtTheEnd2(leaf, last_pos))
    		{
    			auto next_leaf = self.splitLeafP(leaf, Position(last_pos));

    			self.insertDataIntoLeaf(leaf, last_pos, provider);

    			if (provider.hasData())
    			{
    				return self.insertDataRest(leaf, next_leaf, provider);
    			}
    			else {
    				return InsertDataResult(next_leaf, Position());
    			}
    		}
    		else {
    			auto next_leaf = self.getNextNodeP(leaf);

    			if (next_leaf.isSet())
    			{
    				return self.insertDataRest(leaf, next_leaf, provider);
    			}
    			else {
    				return self.insertDataRest(leaf, provider);
    			}
    		}
    	}
    	else {
    		return InsertDataResult(leaf, last_pos);
    	}
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::LeafVariableName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
