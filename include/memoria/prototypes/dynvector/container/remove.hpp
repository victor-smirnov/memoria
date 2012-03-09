
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

    /**
     * Remove the data block between positions pointed with iterators.
     *
     * start - remove from (inclusive)
     *
     * stop  - remove up to (exclusive)
     *
     */

    BigInt RemoveDataBlock(Iterator& start, Iterator& stop);

    bool MergeDataWithSiblings(Iterator& iter);

    bool MergeDataWithRightSibling(Iterator& iter);
    bool MergeDataWithLeftSibling(Iterator& iter);

    bool ShouldMergeData(const DataPageG& data) const
    {
    	return data->data().size() <= DataPage::get_max_size();
    }

    bool CanMergeData(const DataPageG& data1, const DataPageG& data2) const
    {
    	return data1->parent_id() == data2->parent_id()  &&  (data1->data().size() + data2->data().size() <= DataPage::get_max_size());
    }


    NodeBaseG GetDataParent0(const DataPageG& data, const NodeBaseG& other_parent, Int flags) const
    {
//    	if (data->parent_id() == other_parent->id())
//    	{
//    		return other_parent;
//    	}
//    	else {
//    		return me()->GetDataParent(data, flags);
//    	}

    	return NodeBaseG();
    }

    void AddAndSubtractKeyValues(NodeBaseG& start, Int add_idx, NodeBaseG& stop, Int sub_idx, const Key* keys);

private:

    BigInt RemoveData(NodeBaseG& page, DataPageG& data, Int start, Int length);

    struct MergeType {
    	enum Enum {LEFT, RIGHT};
    };

    void MergeDataPagesAndRemoveSource(
    		NodeBaseG& target_parent,
    		DataPageG& target_data,
    		NodeBaseG& source_parent,
    		DataPageG& source_data,
    		typename MergeType::Enum merge_type
    );

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::RemoveDataBlock(Iterator& start, Iterator& stop)
{
   	BigInt pos = start.pos();

   	// FIXME: swap iterators if start is after stop

   	//FIXME: ranges
   	if (!start.IsEof() && pos < stop.pos())
   	{
   		if (start.data()->id() == stop.data()->id())
   		{
   			// Within the same data node
   			// FIXME: Merge with siblings
   			BigInt result = RemoveData(start.page(), start.data(), start.data_pos(), stop.data_pos() - start.data_pos());

   			me()->MergeDataWithSiblings(start);

   			return result;
   		}
   		else {
   			// Removed region crosses data node boundary

   			BigInt removed = 0;

   			DataPageG start_data;
   			DataPageG stop_data;

   			Key start_prefixes[Indexes];
   			for (Int c = 0; c < Indexes; c++) start_prefixes[c] = start.prefix(c);

   			Key start_delta = 0;

   			if (start.data_pos() > 0)
   			{
   				// Remove a region in current data node starting from data_pos till the end
   				Int length 	=  start.data()->data().size() - start.data_pos();

   				removed     += RemoveData(start.page(), start.data(), start.data_pos(), length);

   				start_data 	=  start.data();
   				start_delta =  start_data->size();

   				start.NextKey();
   			}
   			else
   			{
   				//FIXME start_data 				=  start.GetPrevDataPage();

   				if (start_data.is_set())
   				{
   					start_prefixes[0] 	-= start_data->size();
   					start_delta 		=  start_data->size();
   				}
   			}


   			if (!stop.IsEof())
   			{
   				if (stop.data_pos() > 0)
   				{
   					removed 		+= RemoveData(stop.page(), stop.data(), 0, stop.data_pos());
   					stop.data_pos()	=  0;
   				}

   				stop_data = stop.data();
   			}
   			else {
   				stop.NextKey();
   			}

   			BigInt keys[Indexes];
   			me()->ClearKeys(keys);

//   			me()->RemoveEntries(start, stop, keys);

   			removed 			+= keys[0];

   			// FIXME: prefixes
   			start.prefix(0)		=  start_prefixes[0];
   			stop.prefix(0)  	=  start_prefixes[0] + start_delta;

   			stop.data() 		=  stop_data;
   			start.data() 		=  start_data;

   			if (stop_data.is_set())
   			{
   				stop.page()			= me()->GetDataParent0(stop_data, stop.page(), Allocator::READ);
   				stop.key_idx()		= stop_data->parent_idx();
   				stop.data_pos()		= 0;
   			}

   			if (start_data.is_set())
   			{
   				start.page()		= me()->GetDataParent0(start_data, start.page(), Allocator::READ);
   				start.key_idx()		= start_data->parent_idx();
   				start.data_pos() 	= start_data.is_set() ? start_data->data().size() : 0;
   			}


   			if (start_data.is_set() && stop_data.is_set())
   			{
   				if (me()->MergeDataWithSiblings(start))
   				{
   					stop = start;
   				}
   				else {
   					start = stop;
   				}
   			}
   			else if (start_data.is_empty() && stop_data.is_set())
   			{
   				me()->MergeDataWithRightSibling(stop);
   				start = stop;
   			}
   			else if (start_data.is_set() && stop_data.is_empty())
   			{
   				me()->MergeDataWithLeftSibling(start);
   				stop = start;
   			}

   			return removed;
   		}
   	}
   	else {
   		return 0;
   	}
}


M_PARAMS
bool M_TYPE::MergeDataWithRightSibling(Iterator& iter)
{
	DataPageG& data = iter.data();

	DataPageG next_data;// FIXME = iter.GetNextDataPage();
	if (next_data.is_set() && me()->CanMergeData(data, next_data))
	{
		//merge next_data to data and remove next_data
		NodeBaseG next_parent = me()->GetDataParent0(data, iter.page(), Allocator::UPDATE);

		MergeDataPagesAndRemoveSource(next_parent, next_data, iter.page(), data, MergeType::RIGHT);

		// iter.data_pos() doesn't change in this case
		// iter.prefixed() don't change in this case

		iter.page() 	= next_parent;
		iter.data()		= next_data;
		iter.key_idx()	= next_data->parent_idx();

		return true;
	}
	else {
		return false;
	}
}

M_PARAMS
bool M_TYPE::MergeDataWithLeftSibling(Iterator& iter)
{
	DataPageG& data = iter.data();
	DataPageG prev_data; //FIXME = iter.GetPrevDataPage();

	if (prev_data.is_set() && me()->CanMergeData(prev_data, data))
	{
		// 1. merge data to prev_data
		// 2. update iter.data(), iter.page() and iter.data_pos()
		// 3. remove data

		Int data_pos_delta = prev_data->data().size();

		NodeBaseG prev_parent = me()->GetDataParent0(prev_data, iter.page(), Allocator::UPDATE);

		Key prefix = prev_data->size();

		MergeDataPagesAndRemoveSource(prev_parent, prev_data, iter.page(), data, MergeType::LEFT);

		iter.page() 	= prev_parent;
		iter.data()		= prev_data;
		iter.key_idx()	= prev_data->parent_idx();

		iter.data_pos() += data_pos_delta;

		//FIXME: handle multiple keys
		iter.prefix(0) -= prefix;

		return true;
	}
	else {
		return false;
	}
}


M_PARAMS
bool M_TYPE::MergeDataWithSiblings(Iterator& iter)
{
	if (!iter.IsEmpty())
	{
		DataPageG& data = iter.data();

		if (me()->ShouldMergeData(data))
		{
			if (!iter.IsEof())
			{
				if (me()->MergeDataWithRightSibling(iter))
				{
					return true;
				}
				else {
					return me()->MergeDataWithLeftSibling(iter);
				}
			}
			else {
				return me()->MergeDataWithLeftSibling(iter);
			}
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

M_PARAMS
void M_TYPE::AddAndSubtractKeyValues(NodeBaseG& start, Int add_idx, NodeBaseG& stop, Int sub_idx, const Key* keys)
{
	if (start->id() == stop->id())
	{
		// We have reached the root of subtree
		me()->AddAndSubtractKeys(start, add_idx, sub_idx, keys);
	}
	else
	{
		NodeBaseG start_parent 	= me()->GetParent(start, Allocator::UPDATE);
		NodeBaseG stop_parent	= me()->GetNodeParent(stop, start_parent, Allocator::UPDATE);

		AddAndSubtractKeyValues(start_parent, start->parent_idx(), stop_parent, stop->parent_idx(), keys);
	}
}

/////  ------------------------------------------ PRIVATE FUNCTIONS -----------------------

M_PARAMS
BigInt M_TYPE::RemoveData(NodeBaseG& page, DataPageG& data, Int start, Int length)
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

	return length;
}



M_PARAMS
void M_TYPE::MergeDataPagesAndRemoveSource(
		NodeBaseG& target_parent,
		DataPageG& target_data,
		NodeBaseG& source_parent,
		DataPageG& source_data,
		typename MergeType::Enum merge_type
)
{
	source_parent.update();
	target_parent.update();
	target_data.update();

	Int src_size = source_data->size();
	Int tgt_size = target_data->size();

	//FIXME: we have to get all keys values for the moved data block there

	Key keys[Indexes];
	me()->ClearKeys(keys);

	keys[0] = src_size;

	if (merge_type == MergeType::LEFT)
	{
		memoria::CopyBuffer(source_data->data().value_addr(0), target_data->data().value_addr(tgt_size), src_size);

		me()->AddAndSubtractKeyValues(target_parent, target_data->parent_idx(), source_parent, source_data->parent_idx(), keys);
	}
	else {
		// make a room for source data in the target data page
		// FIXME: separate method for this task?
		memoria::CopyBuffer(target_data->data().value_addr(0), target_data->data().value_addr(src_size), tgt_size);

		// copy page content from source to target
		memoria::CopyBuffer(source_data->data().value_addr(0), target_data->data().value_addr(0), src_size);

		me()->NegateKeys(keys);
		me()->AddAndSubtractKeyValues(source_parent, source_data->parent_idx(), target_parent, target_data->parent_idx(), keys);
	}

	target_data->data().size() += src_size;

	source_data->data().size() -= src_size;

	me()->ClearKeys(keys);

	Int source_parent_idx = source_data->parent_idx();
	me()->RemoveEntry(source_parent, source_parent_idx, keys);

	target_parent = me()->GetDataParent0(target_data, target_parent, Allocator::READ);
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
