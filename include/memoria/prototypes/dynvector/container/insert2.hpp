
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

typedef typename Types::DataPage                                        	DataPage;
typedef typename Types::DataPageG                                        	DataPageG;
typedef typename Types::Buffer                                          	Buffer;
typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
typedef typename Types::CountData                                       	CountData;

typedef typename Base::LeafNodeKeyValuePair									LeafNodeKeyValuePair;


static const Int Indexes                                                    = Types::Indexes;
typedef Accumulators<Key, Counters, Indexes>								Accumulator;


void InsertData(Iterator& iter, const ArrayData& data);
DataPageG SplitDataPage(Iterator& iter);


void UpdateParentLinksAndCounters(NodeBaseG& node) const;



private:

void InsertIntoDataPage(Iterator& iter, const ArrayData& buffer);

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
		Int data_size = DataPage::get_max_size();

		suffix_ 	= length % data_size == 0 ? data_size : length_ % data_size;
		last_idx_ 	= Base::GetTotalKeyCount() - 1;
	}

	virtual LeafNodeKeyValuePair GetLeafKVPair(Direction direction, BigInt idx)
	{
		LeafNodeKeyValuePair pair;

		DataPageG data      	= Base::ctr().allocator().CreatePage();
		data->init();

		data->model_hash()      = Base::ctr().hash();
		data->page_type_hash()  = DataPage::hash();

		Int idx0 				= (direction == Direction::FORWARD) ? idx : last_idx_ - idx;

		Int data_size 			= DataPage::get_max_size();

		BigInt offset 			= start_ + data_size * idx0;
		BigInt length 			= idx0 < last_idx_ ? data_size : suffix_;

		CopyBuffer(data_.data() + offset, data->data().value_addr(0), length);

		data->data().size() = length;

		pair.keys[0] 	= length;
		pair.value		= data->id();

		return pair;
	}

};

struct UpdateLeafParentLinksFn
{
	const MyType&	ctr_;
	NodeBaseG&		node_;

	typedef PageGuard<TreePage<Allocator>, Allocator> TreeNodeG;

	UpdateLeafParentLinksFn(const MyType& ctr, NodeBaseG& node):
		ctr_(ctr),
		node_(node)
	{}

	template <typename Node>
	void operator()(Node* node)
	{
		for (Int c = 0; c < node->children_count(); c++)
		{
			ID id 				= node->map().data(c);
			TreeNodeG child 	= ctr_.allocator().GetPage(id, Allocator::UPDATE);

			child->parent_id() 	= node_->id();
			child->parent_idx() = c;
		}

		node->counters().key_count()	= node->children_count();
		node->counters().page_count() 	= 1;
	}
};


void ImportPages(Iterator& iter, const ArrayData& buffer);

DataPageG CreateDataPage(NodeBaseG& node, Int idx);

void MoveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& target_node, DataPageG& target_data);

MEMORIA_CONTAINER_PART_END


#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::Insert2Name)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::InsertData(Iterator& iter, const ArrayData& buffer)
{
	BigInt 		max_datapage_size 	= DataPage::get_max_size();

	BigInt& 	data_idx 			= iter.data_pos();

	BigInt usage = iter.data() != NULL ? iter.data()->size() : 0;

	if (usage + buffer.size() <= max_datapage_size)
	{
		// The target datapage has enough free space to insert into
		InsertIntoDataPage(iter, buffer);
		data_idx += buffer.size();
	}
	else if (!iter.IsEof())
	{
		if (iter.data_pos() > 0)
		{
			SplitDataPage(iter);
			iter.NextData();
		}

		ImportPages(iter, buffer);
	}
	else if (iter.IsEnd())
	{
		ImportPages(iter, buffer);
	}
	else
	{
		//EOF
		iter.key_idx()++;

		iter.data() 	= NULL;
		iter.data_pos() = 0;

		ImportPages(iter, buffer);

		iter.PrevData();
		iter.data_pos() = iter.data()->size();
	}
}


M_PARAMS
typename M_TYPE::DataPageG M_TYPE::SplitDataPage(Iterator& iter)
{
	NodeBaseG& node = iter.page();
	DataPageG& data	= iter.data();
	Int data_pos	= iter.data_pos();

	Int idx = data->parent_idx() + 1;

	NodeBaseG target_node;

	if (me()->GetCapacity(node) == 0)
	{
		Int split_idx = node->children_count() / 2;
		if (idx < split_idx)
		{
			me()->SplitBTreeNode(node, split_idx, 0);
			target_node = node;
		}
		else if (idx  == split_idx)
		{
			target_node = me()->SplitBTreeNode(node, split_idx, 0);
			idx = 0;
		}
		else
		{
			target_node = me()->SplitBTreeNode(node, split_idx, 0);
			idx -= split_idx;

			node = target_node;
			iter.key_idx() = idx - 1;
		}
	}
	else {
		target_node = node;
	}

	me()->InsertSpace(target_node, idx, 1);
	DataPageG target_data = me()->CreateDataPage(target_node, idx);

	MoveData(node, data, data_pos, target_node, target_data);

	return target_data;
}

M_PARAMS
void M_TYPE::UpdateParentLinksAndCounters(NodeBaseG& node) const
{
	if (node->is_leaf())
	{
		UpdateLeafParentLinksFn fn(*me(), node);
		LeafDispatcher::Dispatch(node, fn);
	}
	else {
		Base::UpdateParentLinksAndCounters(node);
	}
}


//// ========================= PRIVATE API =============================== ////

M_PARAMS
void M_TYPE::InsertIntoDataPage(Iterator& iter, const ArrayData& buffer)
{

	DataPageG& data	= iter.data();

	if (data.is_empty())
	{
		me()->InsertSpace(iter.page(), iter.key_idx(), 1);

		data 			= CreateDataPage(iter.page(), iter.key_idx());
		iter.data_pos() = 0;
	}


	Int data_pos	= iter.data_pos();

	data->data().shift(data_pos, buffer.size());

	memoria::CopyBuffer(buffer.data(), data->data().value_addr(data_pos), buffer.size());

	data->data().size() += buffer.size();

	Accumulator accum;

	accum.keys()[0] = buffer.size();

	me()->UpdateUp(iter.page(), data->parent_idx(), accum);
}

M_PARAMS
void M_TYPE::ImportPages(Iterator& iter, const ArrayData& buffer)
{
//	DataPageG& data = iter.data();

//	if (iter.data_pos() == 0)
//	{
//		 if (iter.PrevKey())
//		 {
//			 iter.data_pos() = iter.data()->size();
//		 }
//	}
//	else {
//		me()->SplitDataPage(iter);
//	}

//	if (iter.data_pos() > 0)
//	{
//		me()->SplitDataPage(iter);
//		iter.NextData();
//	}

	BigInt	length		= buffer.size();
	BigInt 	key_count 	= me()->Divide(length, DataPage::get_max_size());
	Int 	start 		= 0;

	ArrayDataSubtreeProvider provider(*me(), key_count, buffer, start, length);

	me()->InsertSubtree(iter, provider);
}

M_PARAMS
typename M_TYPE::DataPageG M_TYPE::CreateDataPage(NodeBaseG& node, Int idx)
{
	DataPageG data      	= me()->allocator().CreatePage();
	data->init();

	data->parent_idx()      = idx;
	data->parent_id()       = node->id();

	data->model_hash()      = me()->hash();
	data->page_type_hash()  = DataPage::hash();

	me()->SetLeafData(node, idx, data->id());

	return data;
}


M_PARAMS
void M_TYPE::MoveData(NodeBaseG& src_node, DataPageG& src_data, Int src_idx, NodeBaseG& tgt_node, DataPageG& tgt_data)
{
	src_data.update();
	tgt_data.update();

	Int amount_to_topy = src_data->size() - src_idx;

	// make a room in the target page
	tgt_data->data().shift(0, amount_to_topy);
	memoria::CopyBuffer(src_data->data().value_addr(src_idx), tgt_data->data().value_addr(0), amount_to_topy);

	Accumulator accum;

	src_data->data().size() -= amount_to_topy;
	tgt_data->data().size() += amount_to_topy;

	accum.keys()[0] = -amount_to_topy;
	me()->UpdateUp(src_node, src_data->parent_idx(), accum);

	accum.keys()[0] = amount_to_topy;
	me()->UpdateUp(tgt_node, tgt_data->parent_idx(), accum);
}

#undef M_PARAMS
#undef M_TYPE

}


#endif
