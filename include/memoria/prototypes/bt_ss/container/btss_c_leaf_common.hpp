
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
    using ReadLeafEntryRtnType = DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<0, SubstreamsIdxList>, GetLeafValuesFn, Args...>;

    template <typename SubstreamsIdxList, typename... Args>
    auto _readLeafEntry(const NodeBaseG& leaf, Args&&... args) const
    {
    	 return self().template _applySubstreamsFn<0, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }


    template <typename LeafPosition>
    bool isAtTheEnd(const NodeBaseG& leaf, LeafPosition pos)
    {
    	Int size = self().template getLeafStreamSize<0>(leaf);
    	return pos >= size;
    }


    bool isAtTheEnd2(const NodeBaseG& leaf, const Position& pos)
    {
    	Int size = self().template getLeafStreamSize<0>(leaf);
    	return pos[0] >= size;
    }


    void insertEntry(Iterator& iter, const InputTuple& entry)
    {
    	self().template insertStreamEntry<0>(iter, entry);
    }


    template <typename SubstreamsList, typename... TupleTypes>
    void updateEntry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
    {
    	self().template updateStreamEntry<0, SubstreamsList>(iter, entry);
    }


    void removeEntry(Iterator& iter) {
    	self().template removeStreamEntry<0>(iter);
    }


    Iterator seek(CtrSizeT position)
    {
    	return self().template _seek<0>(position);
    }


    template <typename OutputIterator>
    CtrSizeT read(Iterator& iter, OutputIterator begin, CtrSizeT length)
    {
    	return 0;
    }

    template <typename InputProvider>
    CtrSizeT insert(Iterator& iter, InputProvider& provider)
    {
    	auto& self = this->self();

    	auto pos = Position(iter.idx());

    	auto id = iter.leaf()->id();

    	auto result = self.insertData(iter.leaf(), pos, provider);

    	iter.leaf() = result.leaf();
    	iter.idx() = result.position()[0];

    	if (id != iter.leaf()->id())
    	{
    		iter.refresh();
    	}

    	return provider.total();
    }


    template <typename LeafPath>
    struct ReadSubstreamFn {
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
    CtrSizeT read_substream(Iterator iter, Fn&& fn, CtrSizeT limit)
    {
    	auto& self 	= this->self();

    	auto& cache = iter.cache();

    	auto pos = iter.pos();

    	CtrSizeT total = 0;

    	while (pos < limit)
    	{
    		auto idx = iter.idx();

    		Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadSubstreamFn<LeafPath>(), std::forward<Fn>(fn), idx, idx + (limit - pos));

    		total += iter.skipFw(processed);

    		pos = iter.pos();
    	}

    	return total;
    }


    struct ReadEntriesFn {

    	template <Int ListIdx, typename StreamObj, typename Entry, typename... Args>
    	auto stream(const StreamObj* obj, Entry&& entry, Args&&... args)
    	{
    		get<ListIdx>(entry) = obj->get_values(std::forward<Args>(args)...);
    	}

    	template <typename Node, typename Fn>
    	Int treeNode(const Node* node, Fn&& fn, Int from, CtrSizeT to)
    	{
    		Int limit = node->size(0);

    		if (to < limit) {
    			limit = to;
    		}

    		for (Int c = from; c < limit; c++)
    		{
    			InputTuple tuple;

    			node->template processStream<IntList<0>>(*this, tuple, c);
    			fn(tuple);
    		}

    		return limit - from;
    	}
    };



    template <typename Fn>
    CtrSizeT read_entries(Iterator& iter, CtrSizeT length, Fn&& fn)
    {
    	CtrSizeT total = 0;

    	while (total < length)
    	{
    		auto idx = iter.idx();

    		Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadEntriesFn(), std::forward<Fn>(fn), idx, idx + (length - total));

    		if (processed > 0) {
    			total += iter.skipFw(processed);
    		}
    		else {
    			break;
    		}
    	}

    	return total;
    }

MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
