
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

    template <typename LeafPath>
    struct ScanFn {
    	template <typename Node, typename Fn>
    	auto treeNode(const Node* node, Fn&& fn, Int from, CtrSizeT to)
    	{
    		auto stream = node->template substream<LeafPath>();
    		Int stream_size = stream->size();

    		Int limit = (to > stream_size) ? stream_size : to;

    		fn(stream, from, limit);

    		return limit - from;
    	}
    };



    template <typename LeafPath, typename Fn>
    CtrSizeT scan(Fn&& fn, CtrSizeT limit = -1)
    {
    	auto& self 	= this->self();
    	auto& cache = self.cache();

    	constexpr Int StreamIdx = ListHead<LeafPath>::Value;

    	MEMORIA_ASSERT(StreamIdx, ==, self.stream());

    	auto size = cache.data_size()[StreamIdx];

    	if (limit == -1 || limit > size) {
    		limit = size;
    	}

    	auto pos = cache.data_pos()[StreamIdx];

    	CtrSizeT total = 0;

    	while (pos < limit)
    	{
    		auto idx 	= self.idx();
    		Int processed = LeafDispatcher::dispatch(self.leaf(), ScanFn<LeafPath>(), std::forward<Fn>(fn), idx, idx + limit);

    		total += self.skipFw(processed);

    		pos = cache.data_pos()[StreamIdx];
    	}

    	return total;
    }


//    void refresh()
//    {
//    	Base::refresh();
//
//    	auto& self 	= this->self();
//    	auto& cache	= self.cache();
//
//    	cache.data_size()[0] = self.ctr().sizes()[0];
//    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

