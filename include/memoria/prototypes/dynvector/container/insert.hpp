
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_INSERT2_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_INSERT2_HPP

#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::InsertName)

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

typedef typename Types::DataPage                                        	DataPage;
typedef typename Types::DataPageG                                        	DataPageG;
typedef typename Types::Buffer                                          	Buffer;
typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
typedef typename Types::CountData                                       	CountData;

typedef typename Types::TreePath                                       		TreePath;
typedef typename Types::TreePathItem                                       	TreePathItem;
typedef typename Types::DataPathItem                                       	DataPathItem;

typedef typename Base::LeafNodeKeyValuePair									LeafNodeKeyValuePair;


static const Int Indexes                                                    = Types::Indexes;
typedef Accumulators<Key, Indexes>											Accumulator;


void insertData(Iterator& iter, const IData& data);
void insertData(Iterator& iter, const IData& data, SizeT start, SizeT len);

BigInt updateData(Iterator& iter, const IData& data, BigInt start, BigInt len);


DataPathItem splitDataPage(Iterator& iter);

Int getMaxDataSize() const
{
	return DataPage::getMaxSize() - DataPage::getMaxSize() % me()->getElementSize();
}




private:

void insertIntoDataPage(Iterator& iter, const IData& buffer, Int start, Int length);

class ArrayDataSubtreeProvider: public MyType::DefaultSubtreeProviderBase {

	typedef typename MyType::DefaultSubtreeProviderBase 	Base;
	typedef typename Base::Direction 						Direction;

	const IData& 		data_;
	BigInt				start_;
	BigInt				length_;
	Int 				suffix_;
	Int					last_idx_;

public:
	ArrayDataSubtreeProvider(MyType& ctr, BigInt key_count, const IData& data, BigInt start, BigInt length):
		Base(ctr, key_count), data_(data), start_(start), length_(length)
	{
		Int data_size 	= Base::ctr().getMaxDataSize();

		suffix_ 		= length % data_size == 0 ? data_size : length_ % data_size;
		last_idx_ 		= Base::getTotalKeyCount() - 1;
	}

	virtual LeafNodeKeyValuePair getLeafKVPair(Direction direction, BigInt idx)
	{
		LeafNodeKeyValuePair pair;

		DataPageG data      	= Base::ctr().allocator().createPage();
		data->init();

		data->model_hash()      = Base::ctr().hash();
		data->page_type_hash()  = DataPage::hash();

		Int idx0 				= (direction == Direction::FORWARD) ? idx : last_idx_ - idx;

		Int data_size 			= Base::ctr().getMaxDataSize();

		BigInt offset 			= start_ + data_size * idx0;
		BigInt length 			= idx0 < last_idx_ ? data_size : suffix_;

		data_.get(data->data().value_addr(0), offset, length);

		data->data().size() = length;

		pair.keys[0] 	= length;
		pair.value		= data->id();

		return pair;
	}

};

void importPages(Iterator& iter, const IData& buffer);

void createDataPage(TreePath& path, Int idx);
DataPathItem createDataPage(NodeBaseG& node, Int idx);

void moveData(TreePath& src, Int src_idx, TreePath& tgt);
void moveData(TreePath& src, Int src_idx, DataPathItem& tgt);

Accumulator moveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data);


MEMORIA_CONTAINER_PART_END








#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::insertData(Iterator& iter, const IData& data, SizeT start, SizeT length)
{
	me()->insertData(iter, GetDataProxy(data, start, length));
}


M_PARAMS
void M_TYPE::insertData(Iterator& iter, const IData& buffer)
{
	BigInt 		max_datapage_size 	= me()->getMaxDataSize();

	BigInt& 	data_idx 			= iter.dataPos();

	BigInt usage = iter.data() != NULL ? iter.data()->size() : 0;

	BigInt pos = iter.pos();

	if (usage + (BigInt)buffer.getSize() <= max_datapage_size)
	{
		// The target datapage has enough free space to insert into
		insertIntoDataPage(iter, buffer, 0, buffer.getSize());
		data_idx += buffer.getSize();
	}
	else if (!iter.isEof())
	{
		if (iter.dataPos() > 0)
		{
			splitDataPage(iter);
		}

		importPages(iter, buffer);

		iter.cache().setup(pos + buffer.getSize() - iter.dataPos(), 0);
	}
	else
	{
		importPages(iter, buffer);

		iter.cache().setup(pos + buffer.getSize() - iter.dataPos(), 0);
	}
}

M_PARAMS
BigInt M_TYPE::updateData(Iterator& iter, const IData& data, BigInt start, BigInt len)
{
	BigInt sum = 0;

	while (len > 0)
	{
		Int to_read = iter.data()->size() - iter.dataPos();

		if (to_read > len) to_read = len;

		data.get(iter.data()->data().value_addr(iter.dataPos()), start, to_read);

		len 	-= to_read;
		iter.skip(to_read);

		sum 	+= to_read;
		start 	+= to_read;

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
	TreePath& path	= iter.path();

	Int data_pos	= iter.dataPos();
	Int idx 		= path.data().parent_idx() + 1;

	if (me()->getCapacity(path[0].node()) == 0)
	{
		TreePath  right = path;
		TreePath& left  = path;

		me()->splitPath(left, right, 0, idx);

		me()->makeRoom(right, 0, 0, 1);

		me()->createDataPage(right, 0);

		me()->reindex(right.leaf());

		moveData(left, data_pos, right);

		return right.data();
	}
	else {
		me()->makeRoom(path, 0, idx, 1);

		DataPathItem target_data = me()->createDataPage(path[0].node(), idx);

		me()->reindex(path.leaf());

		moveData(path, data_pos, target_data);

		return target_data;
	}
}








//// ====================================================== PRIVATE API ============================================================= ////

M_PARAMS
void M_TYPE::insertIntoDataPage(Iterator& iter, const IData& buffer, Int start, Int length)
{
	DataPageG& data	= iter.path().data().node();
	data.update();

	//FIXME: should data page be NULL for empty containers?
	bool reindex_fully = false;
	if (data.isEmpty())
	{
		me()->makeRoom(iter.path(), 0, iter.key_idx(), 1);

		data 			= createDataPage(iter.page(), iter.key_idx());

		iter.path().data().parent_idx() = iter.key_idx();
		iter.dataPos() = 0;
		reindex_fully = true;
	}

	Int data_pos	= iter.dataPos();

	data->data().shift(data_pos, length);

	buffer.get(data->data().value_addr(data_pos), start, length);

	data->data().size() += length;

	Accumulator accum;

	accum.keys()[0] = length;

	me()->updateUp(iter.path(), 0, iter.key_idx(), accum, reindex_fully);
}





M_PARAMS
void M_TYPE::importPages(Iterator& iter, const IData& buffer)
{
	BigInt	length		= buffer.getSize();
	BigInt 	start;

	Int max_size = me()->getMaxDataSize();

	if (iter.dataPos() > 0)
	{
		Int start_page_capacity = max_size - iter.data()->size();

		start = length > start_page_capacity ? start_page_capacity : length;

		insertIntoDataPage(iter, buffer, 0, start);

		iter.nextKey();
	}
	else {
		start = 0;
	}


	BigInt end 			= (length - start) % max_size;

	length -= start + end;

	BigInt key_count	= length / max_size;

	if (key_count > 0)
	{
		ArrayDataSubtreeProvider provider(*me(), key_count, buffer, start, length);
		me()->insertSubtree(iter, provider);
	}

	if (end > 0)
	{
		if (!iter.isEnd())
		{
			Int end_page_capacity = max_size - iter.data()->size();

			if (end <= end_page_capacity)
			{
				insertIntoDataPage(iter, buffer, start + length, end);
				iter.dataPos() = end;
			}
			else {
				ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
				me()->insertSubtree(iter, provider);
				iter.dataPos() = 0;
			}
		}
		else
		{
			ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
			me()->insertSubtree(iter, provider);

			iter.prevKey();
			iter.dataPos() = iter.data()->size();
		}
	}
	else if (iter.isEnd())
	{
		iter.prevKey();
		iter.dataPos() = iter.data()->size();
	}

//	iter.init();
}





M_PARAMS
void M_TYPE::createDataPage(TreePath& path, Int idx)
{
	path.data() = createDataPage(path[0].node(), idx);
}

M_PARAMS
typename M_TYPE::DataPathItem M_TYPE::createDataPage(NodeBaseG& node, Int idx)
{
	DataPageG data      	= me()->allocator().createPage();
	data->init();

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
	me()->updateUp(src, 0, tgt.parent_idx(), 		 accum);
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::moveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data)
{
	src_data.update();
	tgt_data.update();

	Int amount_to_topy = src_data->size() - src_idx;

	// make a room in the target page
	tgt_data->data().shift(0, amount_to_topy);
	memoria::CopyBuffer(src_data->data().value_addr(src_idx), tgt_data->data().value_addr(0), amount_to_topy);

	src_data->data().size() -= amount_to_topy;
	tgt_data->data().size() += amount_to_topy;

	Accumulator accum;

	accum.keys()[0] = amount_to_topy;

	return accum;
}



#undef M_PARAMS
#undef M_TYPE

}


#endif