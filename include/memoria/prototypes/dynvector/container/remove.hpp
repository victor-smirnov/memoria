
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_REMOVE_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_REMOVE_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>





namespace memoria    {


using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::RemoveName)

public:

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Allocator::Page                                            Page;
    typedef typename Page::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::Counters                                            Counters;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
    typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
    typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
    typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
    typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

    typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
    typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Base::Types::DataPage                                  	DataPage;

    static const Int Indexes                                                    = Types::Indexes;

    struct DataRemoveHandlerFn {

    	Int idx_, count_;
    	MyType& me_;

    	DataRemoveHandlerFn(Int idx, Int count, MyType& me): idx_(idx), count_(count), me_(me) {}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		for (Int c = idx_; c <= idx_ + count_; c++)
    		{
    			ID id = node->map().data(c);
    			me_.allocator().RemovePage(id);
    		}
    	}
    };


    bool RemoveDataBlock(Iterator& start, Iterator& stop)
    {
    	if (!start.IsEnd())
    	{
    		Int start_key_idx, stop_key_idx = 0;

    		if (start.idx() > 0)
    		{
    			me_.RemoveData(start.page(), start.data(), start.idx(), start.data()->data().size());
    			start_key_idx = start.key_idx() + 1;
    		}
    		else {
    			start_key_idx = start.key_idx();
    		}


    		if (!stop.IsEnd())
    		{
    			if (stop.idx() > 0)
    			{
    				if (stop.idx() < stop.data()->data().size())
    				{
    					me_.RemoveData(stop.page(), stop.data(), 0, start.idx());
    					//FIXME stop_key_idx = ?????
    				}
    				else {
    					stop_key_idx = stop.key_idx();
    				}
    			}
    			else{
    				stop_key_idx = 0;
    			}
    		}
    		else {
    			stop_key_idx = stop.key_idx() + 1;
    		}

    		Key keys[Indexes] = {0};
    		return me_.RemovePages(start.page(), start_key_idx, stop.page(), stop_key_idx, keys);
    	}
    	else {
    		return false;
    	}
    }

    void RemoveData(NodeBase* page, DataPage* data, Int start, Int stop)
    {

    }

MEMORIA_CONTAINER_PART_END



}



#endif
