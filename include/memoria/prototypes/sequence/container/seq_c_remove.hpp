
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_REMOVE_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/sequence/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::sequence::CtrRemoveName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Allocator::Page                                            Page;
	typedef typename Page::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
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

	typedef typename Types::DataPage                                            DataPage;
	typedef typename Types::DataPageG                                           DataPageG;

	typedef typename Types::IDataSourceType                                     IDataSourceType;
	typedef typename Types::IDataTargetType                                     IDataTargetType;

	typedef typename Types::TreePath                                            TreePath;
	typedef typename Types::TreePathItem                                        TreePathItem;
	typedef typename Types::DataPathItem                                        DataPathItem;

	typedef typename Base::LeafNodeKeyValuePair                                 LeafNodeKeyValuePair;


	static const Int Indexes                                                    = Types::Indexes;
	typedef Accumulators<Key, Indexes>                                          Accumulator;

	typedef typename Types::ElementType                                         ElementType;

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
				me_.allocator().removePage(id);
			}
		}
	};

	/**
	 * remove the data block between positions pointed with iterators.
	 *
	 * start - remove from (inclusive)
	 *
	 * stop  - remove up to (exclusive)
	 *
	 */

	Accumulator removeDataBlock(Iterator& start, Iterator& stop);
	Accumulator removeDataBlock(Iterator& start, BigInt size);

	bool mergeDataWithSiblings(Iterator& iter);

	bool mergeDataWithRightSibling(Iterator& iter);
	bool mergeDataWithLeftSibling(Iterator& iter);



	bool shouldMergeData(const TreePath& path) const
	{
		return path.data()->size() <= me()->getDataPageCapacity(path.data()) / 2;
	}

	bool canMergeData(const TreePath& data1, const TreePath& data2) const
	{
		return data1[0]->id() == data2[0]->id()  &&  (data1.data()->size() + data2.data()->size() <= me()->getDataPageCapacity(data1.data()));
	}

	bool canMergeData2(const TreePath& data1, const TreePath& data2) const
	{
		return data1.data()->size() + data2.data()->size() <= me()->getDataPageCapacity(data1.data());
	}


private:

	Accumulator removeDataBlockFromStart(Iterator& stop);
	Accumulator removeDataBlockAtEnd(Iterator& start);
	Accumulator removeAllData(Iterator& start, Iterator& stop);
	Accumulator removeDataBlockInMiddle(Iterator& start, Iterator& stop);


	Accumulator removeData(TreePath& path, Int start, Int length);

	struct MergeType {
		enum Enum {LEFT, RIGHT};
	};

	void mergeDataPagesAndremoveSource(
			TreePath& target,
			TreePath& source,
			typename MergeType::Enum merge_type
	);

	void mergeDataPagesAndremoveSource(
			TreePath&       target,
			DataPathItem&   target_data,
			TreePath&       source,
			DataPathItem&   source_data,
			typename MergeType::Enum merge_type
	);

	void mergeDataPagesAndremoveSource(
			DataPathItem&   target,
			TreePath&       source,
			typename MergeType::Enum merge_type
	);




MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::sequence::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeDataBlock(Iterator& start, BigInt size)
{
    if (start.pos() + size < start.data()->size())
    {
        // Within the same data node
        Accumulator result = removeData(start.path(), start.dataPos(), size);

        if (me()->mergeDataWithSiblings(start))
        {
            start.init();
        }

        return result;
    }
    else
    {
        auto stop = start;
        stop.skip(size);
        return me()->removeDataBlock(start, stop);
    }
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeDataBlock(Iterator& start, Iterator& stop)
{
    // FIXME: swap iterators if start is after stop

    BigInt start_pos    = start.pos();
    BigInt stop_pos     = stop.pos();

    if (!start.isEof() && start_pos < stop_pos)
    {
        bool at_end     = stop.isEof();

        bool from_start = start.dataPos() == 0 && !start.hasPrevKey();

        if (from_start)
        {
            if (at_end)
            {
                return removeAllData(start, stop);
            }
            else {
                auto result = removeDataBlockFromStart(stop);

                stop.cache().setup(0, 0);

                start = stop;

                return result;
            }
        }
        else
        {
            if (at_end)
            {
                auto result = removeDataBlockAtEnd(start);

                stop.cache().setup(start_pos - stop.dataPos(), 0);

                stop = start;

                return result;
            }
            else {
                return removeDataBlockInMiddle(start, stop);
            }
        }

    }
    else {
        return Accumulator();
    }
}


M_PARAMS
bool M_TYPE::mergeDataWithRightSibling(Iterator& iter)
{
    if (iter.key_idx() < iter.page()->children_count() - 1)
    {
        BigInt source_size = iter.data()->size();

        Int next_idx = iter.key_idx() + 1;

        DataPathItem target_data_item(me()->getValuePage(iter.page(), next_idx, Allocator::READ), next_idx);

        if (source_size <= target_data_item->getCapacity())
        {
            mergeDataPagesAndremoveSource(target_data_item, iter.path(), MergeType::RIGHT);

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

        if (next.nextKey() && me()->canMergeData(next.path(), iter.path()))
        {
            Int data_pos = iter.dataPos();

            mergeDataPagesAndremoveSource(next.path(), iter.path(), MergeType::RIGHT);

            iter = next;

            iter.dataPos() = data_pos;

            return true;
        }
        else {
            return false;
        }

    }
}


M_PARAMS
bool M_TYPE::mergeDataWithLeftSibling(Iterator& iter)
{
    Iterator prev = iter;

    if (prev.prevKey() && canMergeData2(prev.path(), iter.path()))
    {
        Int data_pos = iter.dataPos();
        Int target_data_size = prev.data()->size();

        Int source_node_size = iter.path().leaf()->children_count();

        mergeDataPagesAndremoveSource(prev.path(), iter.path(), MergeType::LEFT);

        if (source_node_size > 1 && me()->shouldMergeNode(iter.path(), 0))
        {
            me()->mergePaths(prev.path(), iter.path());
        }

        iter = prev;
        iter.dataPos() = target_data_size + data_pos;

        return true;
    }
    else {
        return false;
    }
}


M_PARAMS
bool M_TYPE::mergeDataWithSiblings(Iterator& iter)
{
    if (!iter.isEmpty())
    {
        if (me()->shouldMergeData(iter.path()))
        {
            if (!iter.isEof())
            {
                if (me()->mergeDataWithRightSibling(iter))
                {
                    return true;
                }
                else {
                    return me()->mergeDataWithLeftSibling(iter);
                }
            }
            else {
                return me()->mergeDataWithLeftSibling(iter);
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


/////  ------------------------------------------ PRIVATE FUNCTIONS -----------------------

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeDataBlockFromStart(Iterator& stop)
{
    Accumulator removed;

    if (stop.dataPos() > 0)
    {
        removed         += removeData(stop.path(), 0, stop.dataPos());
        stop.dataPos()  =  0;
    }

    BigInt removed_key_count = 0;
    me()->removePagesFromStart(stop.path(), stop.key_idx(), removed, removed_key_count);

    if (me()->shouldMergeData(stop.path()))
    {
        me()->mergeDataWithRightSibling(stop);
    }

    return removed;
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeDataBlockAtEnd(Iterator& start)
{
    Accumulator removed;

    if (start.dataPos() > 0)
    {
        removed             += removeData(start.path(), start.dataPos(), start.data()->size() - start.dataPos());
        start.dataPos() =  start.data()->size();

        //FIXME: optimize, don't jump to the next leaf
        start.nextKey();
    }

    if (!start.isEnd())
    {
        BigInt removed_key_count = 0;
        me()->removePagesAtEnd(start.path(), start.key_idx(), removed, removed_key_count);
    }

    start.prevKey();
    start.dataPos() = start.data()->size();

    if (me()->shouldMergeData(start.path()))
    {
        me()->mergeDataWithLeftSibling(start);
    }

    return removed;
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeAllData(Iterator& start, Iterator& stop)
{
    Accumulator removed;

    BigInt count = 0;
    me()->removeAllPages(start.path(), stop.path(), removed, count);

    start.path().data().node().clear();
    start.path().data().parent_idx() = 0;

    start.cache().setup(0, 0);
    stop = start;

    return removed;
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeDataBlockInMiddle(Iterator& start, Iterator& stop)
{
    BigInt start_pos = start.pos();

    if (start.data()->id() == stop.data()->id())
    {
        // Within the same data node
        Accumulator result = removeData(stop.path(), start.dataPos(), stop.dataPos() - start.dataPos());

        stop.dataPos() = start.dataPos();

        //FIXME: check iterators identity after this block is completed
        if(me()->mergeDataWithSiblings(stop))
        {
            stop.cache().setup(start_pos - stop.dataPos(), 0);

            start = stop;
        }

        return result;
    }
    else {
        // removed region crosses data node boundary

        Accumulator removed;

        if (start.dataPos() > 0)
        {
            // remove a region in current data node starting from data_pos till the end
            Int length  =  start.data()->size() - start.dataPos();

            removed     += removeData(start.path(), start.dataPos(), length);

            start.nextKey();
        }

        if (stop.dataPos() > 0)
        {
            removed         += removeData(stop.path(), 0, stop.dataPos());
            stop.dataPos()  =  0;
        }

        BigInt removed_key_count = 0;
        me()->removePages(start.path(), start.key_idx(), stop.path(), stop.key_idx(), 0, removed, removed_key_count);

        me()->addTotalKeyCount(-removed_key_count);

        me()->mergeDataWithSiblings(stop);

        stop.cache().setup(start_pos - stop.dataPos(), 0);

        start = stop;

        return removed;
    }
}


M_PARAMS
typename M_TYPE::Accumulator M_TYPE::removeData(TreePath& path, Int start, Int length)
{
    DataPageG& data = path.data().node();

    data.update();

    Int pos = start + length;

    Accumulator accum = me()->getDataIndexes(data, start, start + length);

    if (pos < data->size())
    {
        data->shift(pos, -length);
    }

    data->size() -= length;

    data->reindex();

    me()->updateUp(path, 0, path.data().parent_idx(), -accum);

    return accum;
}



M_PARAMS
void M_TYPE::mergeDataPagesAndremoveSource(
        TreePath&       target,
        DataPathItem&   target_data_item,
        TreePath&       source,
        DataPathItem&   source_data_item,
        typename MergeType::Enum merge_type
)
{
    NodeBaseG& target_parent = target[0].node();
    DataPageG& target_data   = target_data_item.node();
    NodeBaseG& source_parent = source[0].node();
    DataPageG& source_data   = source_data_item.node();

    source_parent.update();
    target_parent.update();
    target_data.update();

    Int src_size = source_data->size();
    Int tgt_size = target_data->size();

    //FIXME: we have to get all keys values for the moved data block there

    Accumulator keys = me()->getDataIndexes(source_data, 0, src_size);

    if (merge_type == MergeType::LEFT)
    {
        source_data->copyTo(target_data.page(), 0, tgt_size, src_size);

        me()->updateUp(target, 0, target_data_item.parent_idx(), keys); // FIXME: move this code out of if(){}
        me()->updateUp(source, 0, source_data_item.parent_idx(), -keys);
    }
    else {
        // make a room for source data in the target data page
        // FIXME: separate method for this task?

        target_data->copyTo(target_data.page(), 0, src_size, tgt_size);

        // copy page content from source to target
        source_data->copyTo(target_data.page(), 0, 0, src_size);

        me()->updateUp(target, 0, target_data_item.parent_idx(), keys); // FIXME: move this code out of if(){}
        me()->updateUp(source, 0, source_data_item.parent_idx(), -keys);
    }

    target_data->size() += src_size;
    source_data->size() -= src_size;

    target_data->reindex();
    source_data->reindex();
}




M_PARAMS
void M_TYPE::mergeDataPagesAndremoveSource(
        TreePath& target,
        TreePath& source,
        typename MergeType::Enum merge_type
)
{
    mergeDataPagesAndremoveSource(
            target,
            target.data(),
            source,
            source.data(),
            merge_type
    );

    Int source_parent_idx = source.data().parent_idx();

    Accumulator keys;
    me()->removeEntry(source, source_parent_idx, keys, false);
}


M_PARAMS
void M_TYPE::mergeDataPagesAndremoveSource(
        DataPathItem&   target_data,
        TreePath&       source,
        typename MergeType::Enum merge_type
)
{
    mergeDataPagesAndremoveSource(
            source,
            target_data,
            source,
            source.data(),
            merge_type
    );

    Int source_parent_idx = source.data().parent_idx();

    Accumulator keys;
    me()->removeEntry(source, source_parent_idx, keys, false);

    source.data().parent_idx()  = source_parent_idx;
    source.data().node()        = target_data.node();
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
