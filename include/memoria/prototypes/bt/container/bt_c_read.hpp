
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_READ_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_READ_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::ReadName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                         BranchNodeEntry;
    typedef typename Types::Position                                            Position;
    typedef typename Types::CtrSizeT                                            CtrSizeT;

    template <Int StreamIdx>
    using TemplateInputTuple = typename Types::template StreamInputTuple<0>;

    static const Int Streams                                                    = Types::Streams;



    template <Int StreamIdx>
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
    			TemplateInputTuple<StreamIdx> tuple;

    			node->template processStream<IntList<StreamIdx>>(*this, tuple, c);
    			fn(tuple);
    		}

    		return limit - from;
    	}
    };



    template <Int StreamIdx, typename Fn>
    CtrSizeT read_entries(Iterator& iter, CtrSizeT length, Fn&& fn)
    {
    	CtrSizeT total = 0;

    	while (total < length)
    	{
    		auto idx = iter.idx();

    		Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadEntriesFn<StreamIdx>(), std::forward<Fn>(fn), idx, idx + (length - total));

    		if (processed > 0) {
    			total += iter.skipFw(processed);
    		}
    		else {
    			break;
    		}
    	}

    	return total;
    }

    template <typename SubstreamPath>
    struct ReadSubstreamFn {

    	template <typename StreamObj, typename Fn>
    	auto stream(const StreamObj* obj, Int block, Int from, Int to, Fn&& fn)
    	{
    		obj->read(block, from, to, std::forward<Fn>(fn));
    	}

    	template <typename Node, typename Fn>
    	Int treeNode(const Node* node, Int block, Int from, CtrSizeT to, Fn&& fn)
    	{
    		Int limit = node->size(0);

    		if (to < limit) {
    			limit = to;
    		}

    		node->template processStream<SubstreamPath>(*this, block, from, limit, std::forward<Fn>(fn));

    		return limit - from;
    	}
    };

    template <typename SubstreamPath, typename Fn>
    CtrSizeT read_substream(Iterator& iter, Int block, CtrSizeT length, Fn&& fn)
    {
    	CtrSizeT total = 0;

    	while (total < length)
    	{
    		auto idx = iter.idx();

    		Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadSubstreamFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn));

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

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ReadName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
