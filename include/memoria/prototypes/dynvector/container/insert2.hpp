
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

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::Insert2Name)

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


void InsertData(Iterator& iter, const ArrayData& data);
BigInt UpdateData(Iterator& iter, const ArrayData& data, BigInt start, BigInt len);

DataPathItem SplitDataPage(Iterator& iter);

Int GetMaxDataSize() const
{
	return DataPage::get_max_size() - DataPage::get_max_size() % me()->GetElementSize();
}




private:

void InsertIntoDataPage(Iterator& iter, const ArrayData& buffer, Int start, Int length);

class ArrayDataSubtreeProvider: public MyType::DefaultSubtreeProviderBase {

	typedef typename MyType::DefaultSubtreeProviderBase 	Base;
	typedef typename Base::Direction 						Direction;

	const ArrayData& 	data_;
	BigInt				start_;
	BigInt				length_;
	Int 				suffix_;
	Int					last_idx_;

public:
	ArrayDataSubtreeProvider(MyType& ctr, BigInt key_count, const ArrayData& data, BigInt start, BigInt length):
		Base(ctr, key_count), data_(data), start_(start), length_(length)
	{
		Int data_size 	= Base::ctr().GetMaxDataSize();

		suffix_ 		= length % data_size == 0 ? data_size : length_ % data_size;
		last_idx_ 		= Base::GetTotalKeyCount() - 1;
	}

	virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt idx)
	{
		LeafNodeKeyValuePair pair;

		DataPageG data      	= Base::ctr().allocator().CreatePage();
		data->init();

		data->model_hash()      = Base::ctr().hash();
		data->page_type_hash()  = DataPage::hash();

		Int idx0 				= (direction == Direction::FORWARD) ? idx : last_idx_ - idx;

		Int data_size 			= Base::ctr().GetMaxDataSize();

		BigInt offset 			= start_ + data_size * idx0;
		BigInt length 			= idx0 < last_idx_ ? data_size : suffix_;

		CopyBuffer(data_.data() + offset, data->data().value_addr(0), length);

		data->data().size() = length;

		pair.keys[0] 	= length;
		pair.value		= data->id();

		return pair;
	}

};

void ImportPages(Iterator& iter, const ArrayData& buffer);

void CreateDataPage(TreePath& path, Int idx);
DataPathItem CreateDataPage(NodeBaseG& node, Int idx);

void MoveData(TreePath& src, Int src_idx, TreePath& tgt);
void MoveData(TreePath& src, Int src_idx, DataPathItem& tgt);

Accumulator MoveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data);


MEMORIA_CONTAINER_PART_END








#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::Insert2Name)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::InsertData(Iterator& iter, const ArrayData& buffer)
{
	BigInt 		max_datapage_size 	= me()->GetMaxDataSize();

	BigInt& 	data_idx 			= iter.data_pos();

	BigInt usage = iter.data() != NULL ? iter.data()->size() : 0;

	if (usage + buffer.size() <= max_datapage_size)
	{
		// The target datapage has enough free space to insert into
		InsertIntoDataPage(iter, buffer, 0, buffer.size());
		data_idx += buffer.size();
	}
	else if (!iter.IsEof())
	{
		if (iter.data_pos() > 0)
		{
			SplitDataPage(iter);
		}

		ImportPages(iter, buffer);
	}
	else
	{
		ImportPages(iter, buffer);
	}
}

M_PARAMS
BigInt M_TYPE::UpdateData(Iterator& iter, const ArrayData& data, BigInt start, BigInt len)
{
	BigInt sum = 0;

	while (len > 0)
	{
		Int to_read = iter.data()->size() - iter.data_pos();

		if (to_read > len) to_read = len;

		CopyBuffer(data.data() + start, iter.data()->data().value_addr(iter.data_pos()), to_read);

		len 	-= to_read;
		iter.Skip(to_read);

		sum 	+= to_read;
		start 	+= to_read;

		if (iter.IsEof())
		{
			break;
		}
	}

	return sum;
}




M_PARAMS
typename M_TYPE::DataPathItem M_TYPE::SplitDataPage(Iterator& iter)
{
	TreePath& path	= iter.path();

	Int data_pos	= iter.data_pos();
	Int idx 		= path.data().parent_idx() + 1;

	if (me()->GetCapacity(path[0].node()) == 0)
	{
		TreePath  right = path;
		TreePath& left  = path;

		me()->SplitPath(left, right, 0, idx);

		me()->MakeRoom(right, 0, 0, 1);

		me()->CreateDataPage(right, 0);

		me()->Reindex(right.leaf());

		MoveData(left, data_pos, right);

		return right.data();
	}
	else {
		me()->MakeRoom(path, 0, idx, 1);

		DataPathItem target_data = me()->CreateDataPage(path[0].node(), idx);

		me()->Reindex(path.leaf());

		MoveData(path, data_pos, target_data);

		return target_data;
	}
}








//// ====================================================== PRIVATE API ============================================================= ////

M_PARAMS
void M_TYPE::InsertIntoDataPage(Iterator& iter, const ArrayData& buffer, Int start, Int length)
{
	DataPageG& data	= iter.path().data().node();
	data.update();

	//FIXME: should data page be NULL for empty containers?
	if (data.is_empty())
	{
		me()->MakeRoom(iter.path(), 0, iter.key_idx(), 1);

		data 			= CreateDataPage(iter.page(), iter.key_idx());

		iter.path().data().parent_idx() = iter.key_idx();
		iter.data_pos() = 0;
	}


	Int data_pos	= iter.data_pos();

	data->data().shift(data_pos, length);

	memoria::CopyBuffer(buffer.data() + start, data->data().value_addr(data_pos), length);

	data->data().size() += length;

	Accumulator accum;

	accum.keys()[0] = length;

	me()->UpdateUp(iter.path(), 0, iter.key_idx(), accum, true);
}





M_PARAMS
void M_TYPE::ImportPages(Iterator& iter, const ArrayData& buffer)
{
	BigInt	length		= buffer.size();
	BigInt 	start;

	Int max_size = me()->GetMaxDataSize();

	if (iter.data_pos() > 0)
	{
		Int start_page_capacity = max_size - iter.data()->size();

		start = length > start_page_capacity ? start_page_capacity : length;

		InsertIntoDataPage(iter, buffer, 0, start);

		iter.NextKey();
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
		me()->InsertSubtree(iter, provider);
	}

	if (end > 0)
	{
		if (!iter.IsEnd())
		{
			Int end_page_capacity = max_size - iter.data()->size();

			if (end <= end_page_capacity)
			{
				InsertIntoDataPage(iter, buffer, start + length, end);
				iter.data_pos() = end;
			}
			else {
				ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
				me()->InsertSubtree(iter, provider);
				iter.data_pos() = 0;
			}
		}
		else
		{
			ArrayDataSubtreeProvider provider(*me(), 1, buffer, start + length, end);
			me()->InsertSubtree(iter, provider);

			iter.PrevKey();
			iter.data_pos() = iter.data()->size();
		}
	}
	else if (iter.IsEnd())
	{
		iter.PrevKey();
		iter.data_pos() = iter.data()->size();
	}
}





M_PARAMS
void M_TYPE::CreateDataPage(TreePath& path, Int idx)
{
	path.data() = CreateDataPage(path[0].node(), idx);
}

M_PARAMS
typename M_TYPE::DataPathItem M_TYPE::CreateDataPage(NodeBaseG& node, Int idx)
{
	DataPageG data      	= me()->allocator().CreatePage();
	data->init();

	data->model_hash()      = me()->hash();
	data->page_type_hash()  = DataPage::hash();

	me()->SetLeafData(node, idx, data->id());

	return DataPathItem(data, idx);
}


M_PARAMS
void M_TYPE::MoveData(TreePath& src, Int src_idx, TreePath& tgt)
{
	NodeBaseG& src_node = src[0].node();
	NodeBaseG& tgt_node = tgt[0].node();

	DataPageG& src_data = src.data().node();
	DataPageG& tgt_data = tgt.data().node();

	Accumulator accum = MoveData(src_node, src_data, src_idx, tgt_node, tgt_data);

	me()->UpdateUp(src, 0, src.data().parent_idx(), -accum);
	me()->UpdateUp(tgt, 0, tgt.data().parent_idx(),  accum);
}



M_PARAMS
void M_TYPE::MoveData(TreePath& src, Int src_idx, DataPathItem& tgt)
{
	NodeBaseG& src_node = src[0].node();
	NodeBaseG& tgt_node = src[0].node();

	DataPageG& src_data = src.data().node();
	DataPageG& tgt_data = tgt.node();

	Accumulator accum = MoveData(src_node, src_data, src_idx, tgt_node, tgt_data);

	me()->UpdateUp(src, 0, src.data().parent_idx(), -accum);
	me()->UpdateUp(src, 0, tgt.parent_idx(), 		 accum);
}



M_PARAMS
typename M_TYPE::Accumulator M_TYPE::MoveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data)
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
