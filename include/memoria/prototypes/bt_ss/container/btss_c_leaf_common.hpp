
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_COMMON_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_MODEL_LEAF_COMMON_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::btss::LeafCommonName)

	using Types = TypesType;
	using NodeBaseG = typename Types::NodeBaseG;

	using Iterator = typename Base::Iterator;

	using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
	using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
	using BranchDispatcher = typename Types::Pages::BranchDispatcher;



	using Accumulator 	= typename Types::Accumulator;
	using Position		= typename Types::Position;

    using SplitFn = std::function<Accumulator (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using CtrSizeT = typename Types::CtrSizeT;

    static const Int Streams                                                    = Types::Streams;


    using InputTuple = typename Types::template StreamInputTuple<0>;


    template <typename SubstreamsIdxList, typename... Args>
    auto read_leaf_entry(const NodeBaseG& leaf, Args&&... args) const
    {
    	 return self().template apply_substreams_fn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    bool isAtTheEnd2(const NodeBaseG& leaf, const Position& pos)
    {
    	Int size = self().template getLeafStreamSize<0>(leaf);
    	return pos[0] >= size;
    }


    void insert_entry(Iterator& iter, const InputTuple& entry)
    {
    	self().template insert_stream_entry<0>(iter, entry);
    }


    template <typename SubstreamsList, typename... TupleTypes>
    void update_entry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
    {
    	self().template update_stream_entry<0, SubstreamsList>(iter, entry);
    }


    void removeEntry(Iterator& iter) {
    	self().template remove_stream_entry<0>(iter);
    }



    template <typename InputProvider>
    CtrSizeT insert(Iterator& iter, InputProvider& provider)
    {
    	auto& self = this->self();

    	auto pos = Position(iter.idx());

    	auto id = iter.leaf()->id();

    	auto result = self.insert_provided_data(iter.leaf(), pos, provider);

    	iter.leaf() = result.leaf();
    	iter.idx() = result.position()[0];

    	if (id != iter.leaf()->id())
    	{
    		iter.refresh();
    	}

    	return provider.total();
    }





MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
