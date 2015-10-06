
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BTTL_CTR_MISC_HPP
#define _MEMORIA_PROTOTYPES_BTTL_CTR_MISC_HPP


#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::bttl::MiscName)

    using Types 			= typename Base::Types;

    using NodeBaseG 		= typename Types::NodeBaseG;
    using Iterator  		= typename Base::Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    using Key 				    = typename Types::Key;
    using Value 			    = typename Types::Value;
    using CtrSizeT			  = typename Types::CtrSizeT;

    using Accumulator 		= typename Types::Accumulator;
    using Position 			  = typename Types::Position;

    static const Int Streams = Types::Streams;

    using PageUpdateMgt 	= typename Types::PageUpdateMgr;

    Iterator Begin() {
    	return self().template _seek<0>(0);
    }

    Iterator End() {
    	auto& self = this->self();
    	return self.template _seek<0>(self.sizes()[0]);
    }


    CtrSizeT size() const {
    	return self().sizes()[0];
    }

    Iterator find(Key key)
    {
    	auto iter = self().template _find2GE<IntList<0>>(0, key);

    	iter.cache().data_size()[0] = self().size();
    	iter.cache().data_pos()[0]++;

    	return iter;
    }

    Iterator seek(CtrSizeT pos)
    {
    	auto iter = self().template _seek<0>(pos);

    	auto& cache = iter.cache();

    	cache.data_size()[0] = self().size();
    	cache.data_pos()[0]++;

    	return iter;
    }




MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bttl::MiscName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
