
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_COMMON_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_LEAF_COMMON_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::LeafCommonName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef std::function<Accumulator (NodeBaseG&, NodeBaseG&)>                 SplitFn;
    typedef std::function<void (const Position&, Int)>                          MergeFn;

    typedef typename Types::Source                                              Source;


    static const Int Streams                                                    = Types::Streams;

    template <Int Stream>
    using StreamInputTuple = typename Types::template StreamInputTuple<Stream>;


    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = DispatchConstRtnType<LeafDispatcher, SubstreamsSetNodeFn<Stream, SubstreamsIdxList>, GetLeafValuesFn, Args...>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    auto _readLeafEntry(const NodeBaseG& leaf, Args&&... args) const -> ReadLeafEntryRtnType<Stream, SubstreamsIdxList, Args...>
    {
    	 return self().template _applySubstreamsFn<Stream, SubstreamsIdxList>(leaf, GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    template <Int Stream>
    void insertStreamEntry(Iterator& iter, const StreamInputTuple<Stream>& entry)
    {
    	auto& self = this->self();

    	auto result = self.template tryInsertStreamEntry<Stream>(iter, entry);

    	if (!std::get<0>(result))
    	{
    		iter.split();

    		result = self.template tryInsertStreamEntry<Stream>(iter, entry);

    		if (!std::get<0>(result))
    		{
    			throw Exception(MA_SRC, "Second insertion attempt failed");
    		}
    	}

    	self.updateParent(iter.leaf(), std::get<1>(result));

    	iter.skipFw(1);

    	self.addTotalKeyCount(Position::create(Stream, 1));
    }


    template <Int Stream, typename SubstreamsList, typename... TupleTypes>
    void updateStreamEntry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
    {
    	auto& self      = this->self();

    	auto result = self.template tryUpdateStreamEntry<Stream, SubstreamsList>(iter, entry);

    	if (!std::get<0>(result))
    	{
    		iter.split();

    		result = self.template tryUpdateStreamEntry<Stream, SubstreamsList>(iter, entry);

    		if (!std::get<0>(result))
    		{
    			throw Exception(MA_SRC, "Second insertion attempt failed");
    		}
    	}

    	self.updateParent(iter.leaf(), std::get<1>(result));
    }


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::LeafCommonName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
