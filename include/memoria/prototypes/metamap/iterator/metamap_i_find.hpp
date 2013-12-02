
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_FIND_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_FIND_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrFindName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::IteratorPrefix                           IteratorPrefix;
    typedef typename Container::Types::CtrSizeT                           		CtrSizeT;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;

    static const Int Indexes													= Container::Types::Indexes;
    static const Int Labels														= Container::Types::Labels;
    static const Int HiddenLabels												= Container::Types::HiddenLabels;

    static const Int HiddenLabelsOffset											= Container::Types::HiddenLabelsOffset;
    static const Int LabelsOffset												= Container::Types::LabelsOffset;



    CtrSizeT findKeyGEFw(Int index, CtrSizeT value)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	typename Types::template FindGEWalker<Types> walker(stream, index + 1, index, value);

    	walker.prepare(self);

    	Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }

    CtrSizeT findKeyGEBw(Int index, CtrSizeT value)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	typename Types::template FindBackwardWalker<Types> walker(stream, index + 1, index, value, SearchType::GT);

    	walker.prepare(self);

    	Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }

MEMORIA_ITERATOR_PART_END

}

#endif
