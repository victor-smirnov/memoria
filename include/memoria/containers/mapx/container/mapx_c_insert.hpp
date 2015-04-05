
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPX_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_MAPX_CTR_INSERT_HPP


#include <memoria/containers/mapx/mapx_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/containers/mapx/mapx_tools.hpp>


#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::mapx::CtrInsertName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef ValuePair<Accumulator, Value>                                       Element;

    typedef typename Types::Entry                                               MapEntry;

    template <
    	Int Idx,
    	Int Offset,
    	bool StreamStart
    >
    struct InsertIntoStreamHanlder
    {
    	template <typename SubstreamType, typename AccumulatorItem, typename Entry>
    	void stream(SubstreamType* obj, AccumulatorItem& accum, Int idx, const Entry& entry)
    	{
    		obj->insert(idx, std::get<Idx>(entry));
    		obj->template sum<Offset>(idx, accum);

    		if (StreamStart)
    		{
    			accum[0] += 1;
    		}
    	}
    };


    template <Int Stream, typename Entry>
    struct InsertIntoLeafFn
    {
    	template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, Accumulator& accum, const Entry& entry)
        {
    		using Node = LeafNode<NTypes>;

            node->layout(255);

            node->template processSubstreamsAcc<Stream, InsertIntoStreamHanlder>(accum, idx, entry);
        }
    };

    void insertEntry(Iterator& iter, const MapEntry& entry)
    {
    	Accumulator accum;
        LeafDispatcher::dispatch(iter.leaf(), InsertIntoLeafFn<0, MapEntry>(), iter.idx(), accum, entry);
    }


    template <Int Stream, typename... TupleTypes>
    std::tuple<bool, Accumulator> tryInsertStreamEntry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
    {
    	auto& self = this->self();

    	PageUpdateMgr mgr(self);

    	self.updatePageG(iter.leaf());

    	mgr.add(iter.leaf());

    	try {
    		Accumulator accum;
    		LeafDispatcher::dispatch(iter.leaf(), InsertIntoLeafFn<Stream, std::tuple<TupleTypes...>>(), iter.idx(), accum, entry);
    		return std::make_tuple(true, accum);
    	}
    	catch (PackedOOMException& e)
    	{
    		mgr.rollback();
    		return std::make_tuple(false, Accumulator());
    	}
    }

    template <Int Stream, typename... TupleTypes>
    void insertStreamEntry(Iterator& iter, const std::tuple<TupleTypes...>& entry)
    {
    	auto& self      = this->self();

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

    Iterator findK(BigInt k)
    {
    	memoria::bt1::FindGTForwardWalker2<memoria::bt1::WalkerTypes<Types, IntList<0>>> w(0, k);
    	return self().find0(0, w);
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mapx::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
