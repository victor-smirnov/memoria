
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP_ITER_NAV_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_ITER_NAV_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/smrk_map/smrkmap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_smark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::smrk_map::ItrNavName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;

    BigInt selectFw(BigInt rank_delta, Int mark)
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();
        Int stream  = self.stream();

        MEMORIA_ASSERT(rank_delta, >=, 0);

        typename Types::template SelectFwWalker<Types> walker(stream, mark, rank_delta);

        walker.prepare(self);

        Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

        return walker.finish(self, idx);
    }

    BigInt selectNext(Int mark)
    {
    	auto& self  = this->self();

    	if (!self.isEnd())
    	{
    		Int current_mark = self.mark();
    		return self.selectFw(1 + (current_mark == mark), mark);
    	}
    	else {
    		return 0;
    	}
    }

MEMORIA_ITERATOR_PART_END

}

#endif
