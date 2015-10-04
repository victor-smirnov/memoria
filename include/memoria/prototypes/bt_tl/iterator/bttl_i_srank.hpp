
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_SRANK_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_SRANK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorStreamRankName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 				= typename Container::Types::CtrSizeT;
    using IteratorAccumulator 	= typename Container::Types::IteratorAccumulator;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;


    template <Int StreamIdx>
    using LeafSizesSubstreamPath = typename Container::Types::template LeafSizesSubstreamPath<StreamIdx>;


    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const Int Streams 				= Container::Types::Streams;
    static const Int SearchableStreams 		= Container::Types::SearchableStreams;

    using LeafPrefixRanks = typename Container::Types::LeafPrefixRanks;





MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorStreamRankName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif
