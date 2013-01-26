
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_INSERT_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_SEQ_C_INSERT_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/sequence/names.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::sequence::CtrInsertName)

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


	void insertData(Iterator& iter, IDataSourceType& data);

	BigInt updateData(Iterator& iter, IDataSourceType& data);

	DataPathItem splitDataPage(Iterator& iter);

private:

	Accumulator insertIntoDataPage(Iterator& iter, IDataSourceType& buffer, Int length);

	class ArrayDataSubtreeProvider: public MyType::DefaultSubtreeProviderBase {

	    typedef typename MyType::DefaultSubtreeProviderBase     Base;

	    IDataSourceType&    data_;
	    BigInt              start_;
	    BigInt              length_;
	    Int                 suffix_;
	    Int                 last_idx_;
	    Int                 page_size_;
	    Int                 max_page_capacity_;

	    Accumulator			accumulator_;

	public:
	    ArrayDataSubtreeProvider(MyType& ctr, BigInt key_count, IDataSourceType& data, BigInt start, BigInt length):
	        Base(ctr, key_count), data_(data), start_(start), length_(length)
	    {
	        max_page_capacity_ = Base::ctr().getMaxDataPageCapacity();

	        suffix_         = length % max_page_capacity_ == 0 ? max_page_capacity_ : length_ % max_page_capacity_;
	        last_idx_       = Base::getTotalKeyCount() - 1;

	        page_size_      = ctr.getRootMetadata().page_size();
	    }

	    virtual LeafNodeKeyValuePair getLeafKVPair(BigInt idx)
	    {
	        LeafNodeKeyValuePair pair;

	        DataPageG data          = Base::ctr().allocator().createPage(page_size_);
	        data->init(page_size_);

	        data->model_hash()      = Base::ctr().hash();
	        data->page_type_hash()  = DataPage::hash();

	        Int idx0                = idx;

	        BigInt length           = idx0 < last_idx_ ? max_page_capacity_ : suffix_;

	        BigInt length_local     = length;
	        BigInt accum            = 0;

	        while (length_local > 0)
	        {
	            SizeT processed = data_.get(data->values(), accum, length_local);

	            length_local    -= processed;
	            accum           += processed;
	        }


	        data->size() 	= length;

	        data->reindex();

	        accumulator_ += pair.keys	= Base::ctr().getDataIndexes(data, 0, length);
	        pair.value      			= data->id();

	        return pair;
	    }

	    const Accumulator& accumulator() const
	    {
	    	return accumulator_;
	    }

	};

	Accumulator importPages(Iterator& iter, IDataSourceType& buffer);

	void createDataPage(TreePath& path, Int idx, Int size = -1);
	DataPathItem createDataPage(NodeBaseG& node, Int idx, Int size = -1);

	void moveData(TreePath& src, Int src_idx, TreePath& tgt);
	void moveData(TreePath& src, Int src_idx, DataPathItem& tgt);

	Accumulator moveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data);



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::sequence::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::insertData(Iterator& iter, IDataSourceType& buffer)
{
	BigInt buffer_size = buffer.getRemainder();

    BigInt& data_idx = iter.dataPos();

    BigInt capacity = iter.data().isSet() ? me()->getDataPageCapacity(iter.data()) : me()->getMaxDataPageCapacity();

    BigInt pos = iter.pos();

    if (buffer_size <= capacity)
    {
        // The target datapage has enough free space to insert into
        insertIntoDataPage(iter, buffer, buffer_size);
        data_idx += buffer_size;
    }
    else if (!iter.isEof())
    {
        if (iter.dataPos() > 0)
        {
            splitDataPage(iter);
        }

        Accumulator indexes = importPages(iter, buffer);

        indexes[0] = pos + buffer_size - iter.dataPos();

        iter.cache().setup(indexes);
    }
    else
    {
    	Accumulator indexes = importPages(iter, buffer);

    	indexes[0] = pos + buffer_size - iter.dataPos();

        iter.cache().setup(indexes);
    }
}

M_PARAMS
BigInt M_TYPE::updateData(Iterator& iter, IDataSourceType& data)
{
    BigInt sum = 0;

    SizeT len = data.getRemainder();

    while (len > 0)
    {
        Int pos0 = iter.dataPos();
        Int size = iter.data()->size();

        Int to_read = size - pos0;

        if (to_read > len) to_read = len;

        BigInt to_read_local = to_read;

        Accumulator accum0	= me()->getDataIndexes(iter.data(), pos0, size);

        while (to_read_local > 0)
        {
            Int pos = iter.dataPos();

            SizeT processed = data.get(iter.data()->values(), pos, to_read_local);

            iter.skip(processed);

            to_read_local -= processed;
        }

        iter.data()->reindex();

        Accumulator accum 	= me()->getDataIndexes(iter.data(), pos0, size);

        me()->updateUp(iter.path(), 0, iter.key_idx(), accum - accum0);

        len -= to_read;
        sum += to_read;

        if (iter.isEof())
        {
            break;
        }
    }

    return sum;
}




M_PARAMS
typename M_TYPE::DataPathItem M_TYPE::splitDataPage(Iterator& iter)
{
    TreePath& path  = iter.path();

    Int data_pos    = iter.dataPos();
    Int idx         = path.data().parent_idx() + 1;

    if (me()->getCapacity(path[0].node()) == 0)
    {
        TreePath  right = path;
        TreePath& left  = path;

        me()->splitPath(left, right, 0, idx);

        me()->makeRoom(right, 0, 0, 1);

        me()->createDataPage(right, 0, path.data()->page_size());

        me()->reindex(right.leaf());

        moveData(left, data_pos, right);

        return right.data();
    }
    else {
        me()->makeRoom(path, 0, idx, 1);

        DataPathItem target_data = me()->createDataPage(path[0].node(), idx, path.data()->page_size());

        me()->reindex(path.leaf());

        moveData(path, data_pos, target_data);

        return target_data;
    }
}








//// =============================================== PRIVATE API ===================================================== ////

M_PARAMS
typename M_TYPE::Accumulator M_TYPE::insertIntoDataPage(Iterator& iter, IDataSourceType& buffer, Int length)
{
    DataPageG& data = iter.path().data().node();
    data.update();

    //FIXME: should data page be NULL for empty containers?
    bool reindex_fully = false;
    if (data.isEmpty())
    {
        me()->makeRoom(iter.path(), 0, iter.key_idx(), 1);

        data            = me()->createDataPage(iter.page(), iter.key_idx());

        iter.path().data().parent_idx() = iter.key_idx();
        iter.dataPos()  = 0;
        reindex_fully   = true;
    }

    Int data_pos    = iter.dataPos();

    data->shift(data_pos, length);

    BigInt length_local = length;

    while (length_local > 0)
    {
        SizeT processed = buffer.get(data->values(), data_pos, length_local);

        length_local    -= processed;
        data_pos        += processed;
    }

    data->size() += length;

    data->reindex();

    Int pos = iter.dataPos();
    Accumulator accum = me()->getDataIndexes(data, pos, pos + length);

    me()->updateUp(iter.path(), 0, iter.key_idx(), accum, reindex_fully);

    return accum;
}





M_PARAMS
typename M_TYPE::Accumulator M_TYPE::importPages(Iterator& iter, IDataSourceType& buffer)
{
    BigInt  length      = buffer.getRemainder();
    BigInt  start		= 0;

    Accumulator indexes;

    if (iter.dataPos() > 0)
    {
        Int start_page_capacity =  me()->getDataPageCapacity(iter.data());

        if (start_page_capacity > 0)
        {
        	start = length > start_page_capacity ? start_page_capacity : length;
        	indexes = insertIntoDataPage(iter, buffer, start);
        }

        iter.nextKey();
    }

    Int max_size = me()->getMaxDataPageCapacity();

    BigInt end          = (length - start) % max_size;

    length              -= start + end;

    BigInt key_count    = length / max_size;

    if (key_count > 0)
    {
        ArrayDataSubtreeProvider provider(*me(), key_count, buffer, start, length);
        me()->insertSubtree(iter, provider);

        indexes += provider.accumulator();
    }

    if (end > 0)
    {
        if (!iter.isEnd())
        {
            Int end_page_capacity = me()->getDataPageCapacity(iter.data());

            if (end <= end_page_capacity)
            {
                indexes += insertIntoDataPage(iter, buffer, end);

                iter.dataPos() = end;
            }
            else {
                ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
                me()->insertSubtree(iter, provider);
                iter.dataPos() = 0;

                indexes += provider.accumulator();
            }
        }
        else
        {
            ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
            me()->insertSubtree(iter, provider);

            indexes += provider.accumulator();

            iter.prevKey();
            iter.dataPos() = iter.data()->size();
        }
    }
    else if (iter.isEnd())
    {
        iter.prevKey();
        iter.dataPos() = iter.data()->size();
    }

    return indexes;
}





M_PARAMS
void M_TYPE::createDataPage(TreePath& path, Int idx, Int size)
{
    path.data() = createDataPage(path[0].node(), idx, size);
}

M_PARAMS
typename M_TYPE::DataPathItem M_TYPE::createDataPage(NodeBaseG& node, Int idx, Int size)
{
    Int page_size           = me()->getRootMetadata().page_size();

    if (size == -1)
    {
        size = page_size;
    }

    DataPageG data          = me()->allocator().createPage(size);
    data->init(size);

    data->model_hash()      = me()->hash();
    data->page_type_hash()  = DataPage::hash();

    me()->setLeafData(node, idx, data->id());

    return DataPathItem(data, idx);
}


M_PARAMS
void M_TYPE::moveData(TreePath& src, Int src_idx, TreePath& tgt)
{
    NodeBaseG& src_node = src[0].node();
    NodeBaseG& tgt_node = tgt[0].node();

    DataPageG& src_data = src.data().node();
    DataPageG& tgt_data = tgt.data().node();

    Accumulator accum = moveData(src_node, src_data, src_idx, tgt_node, tgt_data);

    me()->updateUp(src, 0, src.data().parent_idx(), -accum);
    me()->updateUp(tgt, 0, tgt.data().parent_idx(),  accum);
}



M_PARAMS
void M_TYPE::moveData(TreePath& src, Int src_idx, DataPathItem& tgt)
{
    NodeBaseG& src_node = src[0].node();
    NodeBaseG& tgt_node = src[0].node();

    DataPageG& src_data = src.data().node();
    DataPageG& tgt_data = tgt.node();

    Accumulator accum = moveData(src_node, src_data, src_idx, tgt_node, tgt_data);

    me()->updateUp(src, 0, src.data().parent_idx(), -accum);
    me()->updateUp(src, 0, tgt.parent_idx(),         accum);
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::moveData(
                    NodeBaseG& src_node,
                    DataPageG& src_data,
                    Int src_idx,
                    NodeBaseG& tgt_node,
                    DataPageG& tgt_data
                )
{
    src_data.update();
    tgt_data.update();

    Int amount_to_topy = src_data->size() - src_idx;

    MEMORIA_ASSERT(tgt_data->getCapacity(), >= , amount_to_topy);

    // make a room in the target page
    tgt_data->shift(0, amount_to_topy);

    Accumulator accum = me()->getDataIndexes(src_data, src_idx, src_idx + amount_to_topy);

    src_data->copyTo(tgt_data.page(), src_idx, 0, amount_to_topy);

    src_data->size() -= amount_to_topy;
    tgt_data->size() += amount_to_topy;

    src_data->reindex();
    tgt_data->reindex();

    return accum;
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
