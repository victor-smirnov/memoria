
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
    struct AccumulatorHandler
    {
        template <Int StreamIdx, typename StreamType, typename TupleItem>
        void stream(const StreamType* obj, TupleItem& accum, Int idx, const MapEntry& entry)
        {
        	std::cout<<"Idx "<<Idx<<" "<<Offset<<" "<<StreamIdx<<" "<<StreamStart<<" "<<std::get<StreamIdx>(entry)<<std::endl;
        }
    };


    struct HandlerT
    {
        template <Int AllocIdx, Int StreamIdx, typename StreamType>
        int stream(StreamType* obj)
        {
        	std::cout<<"AllocIdx (Int) "<<AllocIdx<<" "<<StreamIdx<<" "<<std::endl;

        	return 10;
        }
    };

    struct Handler
    {
    	template <Int StreamIdx, typename StreamType>
    	bool stream(StreamType* obj)
    	{
    		std::cout<<"Idx (bool) "<<StreamIdx<<" "<<std::endl;

    		return true;
    	}
    };

    struct HandlerS
    {
    	template <typename StreamType>
    	double stream(StreamType* obj)
    	{
    		std::cout<<"NoIdx (double)"<<" "<<std::endl;

    		return 1.23456;
    	}
    };


    template <typename Entry>
    struct InsertIntoLeafFn {
        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx, Accumulator& accum, const Entry& entry)
        {
            node->layout(255);

            ListPrinter<TypeList<decltype(node->template processStreamsStart(HandlerS()))>>::print(cout);


//            node->template processSubstreamsAcc<0, AccumulatorHandler>(accum, idx, entry);
//
//            auto r = node->template processStream<IntList<0>>(Handler());

//            cout<<"Result: "<<r<<endl;

//            ListPrinter<TL<decltype(r)>>::print(cout);

//            node->processStreamsStartT(HandlerT());

//            node->template processSubstreams<IntList<0>>(*this, idx, entry);
        }

        template <Int Idx, typename SubstreamStruct>
        void stream(SubstreamStruct* obj, Int idx, const Entry& entry)
        {
            std::cout<<"Idx "<<Idx<<" "<<std::get<Idx>(entry)<<std::endl;
        }
    };

    void insertEntry(Iterator& iter, const MapEntry& entry)
    {
    	Accumulator accum;
        LeafDispatcher::dispatch(iter.leaf(), InsertIntoLeafFn<MapEntry>(), iter.idx(), accum, entry);
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mapx::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
