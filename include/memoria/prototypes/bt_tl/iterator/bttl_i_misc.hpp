
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_MISC_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_MISC_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>



#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorMiscName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 	= typename Container::Types::CtrSizeT;
    using Key		= typename Container::Types::Key;
    using Value		= typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;


    template <Int Stream>
    using StreamSizesPath = typename Select<Stream, typename Container::Types::StreamsSizes>::Result;

    static const Int Streams 				= Container::Types::Streams;
    static const Int SearchableStreams 		= Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;


    void update_leaf_ranks()
    {
//    	auto& self = this->self();
//    	auto& prefixes = self.cache().ranks();
//    	self.compute_leaf_prefixes(prefixes);
    }

    void update_leaf_ranks(WalkCmd cmd)
    {
    	if (cmd == WalkCmd::LAST_LEAF){
    		update_leaf_ranks();
    	}
    }


    struct UpdateSizeFn {
    	template <Int Stream, typename Itr, typename Size>
    	void process(Itr&& iter, Int stream, Int idx, Size&& size)
    	{
    		if (iter.isContent(idx))
    		{
    			size[stream + 1] = iter.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}
    };

    struct UpdateSizeElseFn {
    	template <Int Stream, typename Itr, typename Size>
    	void process(Itr&& iter, Int stream, Int idx, Size&& size){}
    };

    template <Int Stream, typename... Args>
    void _update_substream_size(Args&&... args)
    {
    	IfLess<Stream, SearchableStreams>::process(UpdateSizeFn(), UpdateSizeElseFn(), self(), std::forward<Args>(args)...);
    }





MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

