
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

    typedef typename Types::TreePath                                       		TreePath;
    typedef typename Types::TreePathItem                                       	TreePathItem;
    typedef typename Types::DataPathItem                                       	DataPathItem;

    static const Int Indexes                                                    = Types::Indexes;
    typedef Accumulators<Key, Indexes>											Accumulator;

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

    Accumulator RemoveDataBlock(Iterator& start, Iterator& stop);

    bool MergeDataWithSiblings(Iterator& iter);

    bool MergeDataWithRightSibling(Iterator& iter);
    bool MergeDataWithLeftSibling(Iterator& iter);

    bool ShouldMergeData(const TreePath& path) const
    {
    	return path.data()->size() <= DataPage::get_max_size() / 2;
    }

    bool CanMergeData(const TreePath& data1, const TreePath& data2) const
    {
    	return data1[0]->id() == data2[0]->id()  &&  (data1.data()->size() + data2.data()->size() <= DataPage::get_max_size());
    }



    void AddAndSubtractKeyValues(TreePath& start, Int add_idx, TreePath& stop, Int sub_idx, const Accumulator& keys, Int level = 0);

private:

    Accumulator RemoveDataBlockFromStart(Iterator& stop);
    Accumulator RemoveDataBlockAtEnd(Iterator& start);
    Accumulator RemoveAllData(Iterator& start, Iterator& stop);
    Accumulator RemoveDataBlockInMiddle(Iterator& start, Iterator& stop);


    Accumulator RemoveData(TreePath& path, Int start, Int length);

    struct MergeType {
    	enum Enum {LEFT, RIGHT};
    };

    void MergeDataPagesAndRemoveSource(
    		TreePath& target,
    		TreePath& source,
    		typename MergeType::Enum merge_type
    );

    void MergeDataPagesAndRemoveSource(
    		TreePath& 		target,
    		DataPathItem&	target_data,
    		TreePath& 		source,
    		DataPathItem&	source_data,
    		typename MergeType::Enum merge_type
    );

    void MergeDataPagesAndRemoveSource(
    		DataPathItem& 	target,
    		TreePath& 		source,
    		typename MergeType::Enum merge_type
    );

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::RemoveName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveDataBlock(Iterator& start, Iterator& stop)
{
	// FIXME: swap iterators if start is after stop

	if (!start.IsEof() && start.pos() < stop.pos())
	{
		bool at_end 	= stop.IsEof();

		bool from_start = start.data_pos() == 0 && !start.HasPrevKey();

		if (from_start)
		{
			if (at_end)
			{
				return RemoveAllData(start, stop);
			}
			else {
				auto result = RemoveDataBlockFromStart(stop);
				start = stop;
				return result;
			}
		}
		else
		{
			if (at_end)
			{
				auto result = RemoveDataBlockAtEnd(stop);
				start = stop;
				return result;
			}
			else {
				return RemoveDataBlockInMiddle(start, stop);
			}
		}

	}
	else {
		return Accumulator();
	}
}


M_PARAMS
bool M_TYPE::MergeDataWithRightSibling(Iterator& iter)
{
	if (iter.key_idx() < iter.page()->children_count() - 1)
	{
		BigInt source_size = me()->GetKey(iter.page(), 0, iter.key_idx());
		BigInt target_size = me()->GetKey(iter.page(), 0, iter.key_idx() + 1);
		if (source_size + target_size <= DataPage::get_max_size()) //iter.data()->size()
		{
			DataPathItem target_data_item(me()->GetDataPage(iter.page(), iter.key_idx() + 1, Allocator::READ), iter.key_idx() + 1);

			MergeDataPagesAndRemoveSource(target_data_item, iter.path(), MergeType::RIGHT);

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Iterator next = iter;

		if (next.NextKey() && me()->CanMergeData(next.path(), iter.path()))
		{
			Int data_pos = iter.data_pos();

			MergeDataPagesAndRemoveSource(next.path(), iter.path(), MergeType::RIGHT);

			iter = next;

			iter.data_pos() = data_pos;

			return true;
		}
		else {
			return false;
		}

	}
}


M_PARAMS
bool M_TYPE::MergeDataWithLeftSibling(Iterator& iter)
{
	Iterator prev = iter;

	if (prev.PrevKey() && me()->CanMergeData(prev.path(), iter.path()))
	{
		Int data_pos = iter.data_pos();

		MergeDataPagesAndRemoveSource(prev.path(), iter.path(), MergeType::LEFT);

		iter = prev;

		iter.data_pos() = iter.data()->size() + data_pos;

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
		if (me()->ShouldMergeData(iter.path()))
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
void M_TYPE::AddAndSubtractKeyValues(TreePath& start, Int add_idx, TreePath& stop, Int sub_idx, const Accumulator& keys, Int level)
{
	if (start[level]->id() == stop[level]->id())
	{
		// We have reached the root of subtree
		me()->AddAndSubtractKeys(start, level, add_idx, sub_idx, keys);
	}
	else
	{
		AddAndSubtractKeyValues(start, start[level].parent_idx(), stop, stop[level].parent_idx(), keys, level + 1);
	}
}



/////  ------------------------------------------ PRIVATE FUNCTIONS -----------------------

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveDataBlockFromStart(Iterator& stop)
{
	Accumulator removed;

	if (stop.data_pos() > 0)
	{
		removed 		+= RemoveData(stop.path(), 0, stop.data_pos());
		stop.data_pos()	=  0;
	}

	BigInt removed_key_count = 0;
	me()->RemovePagesFromStart(stop.path(), stop.key_idx(), removed, removed_key_count);

	stop.prefix().Clear();

	if (me()->ShouldMergeData(stop.path()))
	{
		me()->MergeDataWithRightSibling(stop);
	}

	return removed;
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveDataBlockAtEnd(Iterator& start)
{
	Accumulator removed;

	TreePath data_path = start.path();

	if (start.data_pos() > 0)
	{
		removed 			+= RemoveData(start.path(), 0, start.data_pos());
		start.data_pos()	=  start.data()->size();

		start.NextKey();
	}

	if (!start.IsEnd())
	{
		BigInt removed_key_count = 0;
		me()->RemovePagesAtEnd(start.path(), start.key_idx(), removed, removed_key_count);
	}

	if (me()->ShouldMergeData(start.path()))
	{
		me()->MergeDataWithLeftSibling(start);
	}

	return removed;
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveAllData(Iterator& start, Iterator& stop)
{
	Accumulator removed;

	BigInt count = 0;
	me()->RemoveAllPages(start.path(), stop.path(), removed, count);

	start.path().data().node().Clear();
	start.path().data().parent_idx() = 0;

	stop = start;

	return removed;
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveDataBlockInMiddle(Iterator& start, Iterator& stop)
{
	if (start.data()->id() == stop.data()->id())
	{
		// Within the same data node
		Accumulator result = RemoveData(stop.path(), start.data_pos(), stop.data_pos() - start.data_pos());

		stop.data_pos() = start.data_pos();

		if (me()->MergeDataWithSiblings(start))
		{
			stop = start;
		}

		return result;
	}
	else {
		// Removed region crosses data node boundary

		Accumulator removed;
		Accumulator start_delta;

		if (start.data_pos() > 0)
		{
			// Remove a region in current data node starting from data_pos till the end
			Int length 	=  start.data()->size() - start.data_pos();

			removed     += RemoveData(start.path(), start.data_pos(), length);

			start_delta.keys()[0] = start.data_pos();

			start.NextKey();
		}

		if (stop.data_pos() > 0)
		{
			removed 		+= RemoveData(stop.path(), 0, stop.data_pos());
			stop.data_pos()	=  0;
		}

		BigInt removed_key_count = 0;
		me()->RemovePages(start.path(), start.key_idx(), stop.path(), stop.key_idx(), 0, removed, removed_key_count);

		stop.prefix() = start.prefix() + start_delta;

		me()->MergeDataWithSiblings(stop);

		start = stop;

		return removed;
	}
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::RemoveData(TreePath& path, Int start, Int length)
{
	DataPageG& data = path.data().node();

	data.update();

	Int pos = start + length;

	if (pos < data->size())
	{
		data->data().shift(pos, -length);
	}

	data->data().size() -= length;

	Accumulator accum;
	accum.keys()[0] = -length;

	me()->UpdateUp(path, 0, path.data().parent_idx(), accum);

	return accum;
}



M_PARAMS
void M_TYPE::MergeDataPagesAndRemoveSource(
		TreePath& 		target,
		DataPathItem&	target_data_item,
		TreePath& 		source,
		DataPathItem&	source_data_item,
		typename MergeType::Enum merge_type
)
{
	NodeBaseG& target_parent = target[0].node();
	DataPageG& target_data	 = target_data_item.node();
	NodeBaseG& source_parent = source[0].node();
	DataPageG& source_data	 = source_data_item.node();

	source_parent.update();
	target_parent.update();
	target_data.update();

	Int src_size = source_data->size();
	Int tgt_size = target_data->size();

	//FIXME: we have to get all keys values for the moved data block there

	Accumulator keys;

	keys.keys()[0] = src_size;

	if (merge_type == MergeType::LEFT)
	{
		memoria::CopyBuffer(source_data->data().value_addr(0), target_data->data().value_addr(tgt_size), src_size);

		me()->AddAndSubtractKeyValues(target, target_data_item.parent_idx(), source, source_data_item.parent_idx(), keys);
	}
	else {
		// make a room for source data in the target data page
		// FIXME: separate method for this task?
		memoria::CopyBuffer(target_data->data().value_addr(0), target_data->data().value_addr(src_size), tgt_size);

		// copy page content from source to target
		memoria::CopyBuffer(source_data->data().value_addr(0), target_data->data().value_addr(0), src_size);

		me()->AddAndSubtractKeyValues(source, source_data_item.parent_idx(), target, target_data_item.parent_idx(), -keys);
	}

	target_data->data().size() += src_size;

	source_data->data().size() -= src_size;

	keys.Clear();
}




M_PARAMS
void M_TYPE::MergeDataPagesAndRemoveSource(
		TreePath& target,
		TreePath& source,
		typename MergeType::Enum merge_type
)
{
	MergeDataPagesAndRemoveSource(
			target,
			target.data(),
			source,
			source.data(),
			merge_type
	);

	Int source_parent_idx = source.data().parent_idx();

	Accumulator keys;
	me()->RemoveEntry(source, source_parent_idx, keys);
}


M_PARAMS
void M_TYPE::MergeDataPagesAndRemoveSource(
		DataPathItem& 	target_data,
		TreePath& 		source,
		typename MergeType::Enum merge_type
)
{
	MergeDataPagesAndRemoveSource(
			source,
			target_data,
			source,
			source.data(),
			merge_type
	);

	Int source_parent_idx = source.data().parent_idx();

	Accumulator keys;
	me()->RemoveEntry(source, source_parent_idx, keys);
}


#undef M_TYPE
#undef M_PARAMS


}



#endif
