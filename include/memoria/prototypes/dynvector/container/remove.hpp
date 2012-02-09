
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
    typedef typename Types::NodeBaseG                                           NodeBaseG;
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
    typedef typename Base::Types::DataPageG                                  	DataPageG;

    static const Int Indexes                                                    = Types::Indexes;

    struct DataRemoveHandlerFn {

    	Int idx_, count_;
    	MyType& me_;

    	DataRemoveHandlerFn(Int idx, Int count, MyType& me): idx_(idx), count_(count), me_(me) {}

    	template <typename Node>
    	void operator()(Node* node)
    	{
    		for (Int c = idx_; c < idx_ + count_; c++)
    		{
    			ID id = node->map().data(c);
    			me_.allocator().RemovePage(id);
    		}
    	}
    };


    bool RemoveDataBlock(Iterator& start, Iterator& stop)
    {
    	BigInt pos = start.pos();

    	if (!start.IsEof() && pos < stop.pos())
    	{
    		if (start.data()->id() == stop.data()->id())
    		{
    			// Withing the same data node
    			// FIXME: Merge with siblings
    			bool result = me()->RemoveData(start.page(), start.data(), start.data_pos(), stop.data_pos() - start.data_pos());

    			stop.data_pos() = start.data_pos();

    			return result;
    		}
    		else {
    			// Removed region crosses data node boundary

    			Int start_key_idx, stop_key_idx = 0;

    			bool removed = false;

    			if (start.data_pos() > 0)
    			{
    				// Remove a region in current data node starting from data_pos till the end
    				removed = me()->RemoveData(start.page(), start.data(), start.data_pos(), start.data()->data().size() - start.data_pos());

    				start_key_idx = start.key_idx();
    			}
    			else {
    				start_key_idx = start.key_idx() - 1;
    			}

    			if (!stop.IsEof())
    			{
    				if (stop.data_pos() > 0)
    				{
    					removed = me()->RemoveData(stop.page(), stop.data(), 0, stop.data_pos()) || removed;
    				}

    				stop_key_idx = stop.key_idx();
    			}
    			else {
    				stop_key_idx = stop.key_idx() + 1;
    			}

    			Key keys[Indexes];
    			for (Int c = 0; c < Indexes; c++) keys[c] = 0;

    			removed = me()->RemovePages(start.page(), start_key_idx, stop.page(), stop_key_idx, keys) || removed;

    			//FIXME: preserve iterator states directly, don't use Seek()
    			Iterator i = me()->Seek(pos);
    			start = i;
    			stop  = i;

    			return removed;
    		}
    	}
    	else {
    		return false;
    	}
    }

    bool RemoveData(NodeBaseG& page, DataPageG& data, Int start, Int length)
    {
    	data.update();

    	Int pos = start + length;

    	if (pos < data->data().size())
    	{
    		data->data().shift(pos, -length);
    	}

    	data->data().size() -= length;

    	BigInt keys[Indexes];
    	for (Int c = 1; c < Indexes; c++) keys[c] = 0;
    	keys[0] = -length;

    	me()->UpdateBTreeKeys(page, data->parent_idx(), keys, true);

    	return length > 0; // FIXME: it's always true;
    }

MEMORIA_CONTAINER_PART_END



}



#endif
