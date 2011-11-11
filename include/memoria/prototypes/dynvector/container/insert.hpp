
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_INSERT_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_INSERT_HPP



#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::InsertName)
private:

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

    typedef typename Types::DataPage                                        	DataPage;
    typedef typename Types::Buffer                                         	 	Buffer;
    typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
    typedef typename Types::CountData                                       	CountData;


    static const Int Indexes                                                    = Types::Indexes;


//PUBLIC API:

    void InsertDataBlock(Iterator &iter, Buffer &block, BufferContentDescriptor &descriptor);


//PROTECTED API:
    DataPage* create_datapage(NodeBase* node, Int idx);


private:

    void import_pages(
    		Iterator &iter,
    		Buffer &block,
    		BufferContentDescriptor &descriptor
    );

    void import_several_pages(
            NodeBase *node,
            BigInt key_idx,
            const Buffer &block,
            BufferContentDescriptor &descriptor,
            BigInt page_count);


    // FIXME: check: make a room for inserted data if necessary
    void import_small_block(
            NodeBase *node,
            BigInt idx,
            BigInt pos,
            const Buffer &block,
            BufferContentDescriptor &descriptor
    );


    void import_data(
                DataPage *page,
                BigInt idx,
                BigInt pos,
                const Buffer &data,
                BufferContentDescriptor &descriptor);

    void move_data_in_page_create(Iterator &iter, BigInt local_idx, CountData &prefix);

    void move_data_in_page_create(DataPage *from, NodeBase *node, BigInt idx, BigInt local_idx, CountData &prefix);

    void move_data_in_page(Iterator &iter, BigInt local_idx, CountData &prefix);

    void move_data_in_page(DataPage *from, DataPage *to, BigInt local_idx, CountData &prefix);

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::InsertDataBlock(Iterator &iter, Buffer &block, BufferContentDescriptor &descriptor)
{
	NodeBase*& 	page 				= iter.page();
	BigInt 		max_datapage_size 	= DataPage::get_max_size();

	MEMORIA_TRACE(me_, "iterator is not empty", block.size(), page, me_.root());

	BigInt& data_idx = iter.data_pos();

	if (iter.IsEmpty())
	{
		iter.key_idx() 	= 0;
		iter.data_pos()		= 0;
		iter.data() 	= me_.InsertDataPage(page, iter.key_idx());
	}

	BigInt usage = iter.data() != nullptr ? me_.GetKey(page, 0, iter.data()->parent_idx()) : 0;

	if (usage + descriptor.length() <= max_datapage_size)
	{
		// The target datapage has enough free space to inset to
		import_small_block(iter.page(), iter.data()->parent_idx(), data_idx, block, descriptor);
		data_idx += descriptor.length();
	}
	else if (!iter.IsEnd())
	{
		if (data_idx > 0 && data_idx < usage)
		{
			// import_data() doesn't move data in the datapage.
			// There are only two such cases:
			// 1. insertion into 0-th position of the datapage
			// 2. insertion into the first free position of the datapage
			//
			// The following code block prepares datapage at the insertion point
			// by moving data into the next datapage if it has free space.
			// Or create a new datapage otherwise.

			DataPage* next_data = iter.GetNextDataPage(page, iter.data());

			if (next_data != NULL)
			{
				BigInt next_data_free_space = max_datapage_size - next_data->data().size();
				BigInt move_amount = usage - data_idx;

				MEMORIA_TRACE(me_, "NextData is not NULL. page.capacity", me_.GetCapacity(page), iter.key_idx());

				if (next_data_free_space < move_amount)
				{
					// There is no enough room in the next datapage
					// Create a new datapage to move data into
					// Split btree if nesessary

					// TODO: Implement as split_datapage()/merge_datapage() methods

					BigInt next_key_idx = iter.key_idx() + 1;

					NodeBase* next_page = page;
					if (me_.GetCapacity(page) == 0)
					{
						if (next_key_idx < me_.GetMaxCapacity(page))
						{
							me_.SplitBTreeNode(next_page, next_key_idx);
						}
						else {
							next_page = me_.SplitBTreeNode(next_page, next_key_idx);
							next_key_idx = 0;
						}
					}

					for (Int c = 0; c < me_.GetChildrenCount(next_page); c++)
					{
						DataPage* data = me_.GetDataPage(next_page, c);
						MEMORIA_TRACE(me_, "xc=", c, "pidx=", data->parent_idx(), data->id());
					}

					next_data = me_.InsertDataPage(next_page, next_key_idx);

					//        				return;
				}
			}
			else if (me_.GetCapacity(page) > 0)
			{
				MEMORIA_TRACE(me_, "NextData is NULL");
				next_data = me_.InsertDataPage(page, iter.key_idx() + 1);
			}
			else
			{
				NodeBase* new_node = me_.SplitBTreeNode(page, iter.key_idx() + 1);
				next_data = me_.InsertDataPage(new_node, 0);
			}

			//FIXME: move base_prefix to the iterator?
			CountData &base_prefix = descriptor.base_prefix();
			me_.move_data_in_page(iter.data(), next_data, data_idx, base_prefix);
		}

		import_pages(iter, block, descriptor);
	}
	else
	{
		MEMORIA_TRACE(me_, "EOF");
		import_pages(iter, block, descriptor);
	}


//	iter.SetStart(false);
//	iter.SetEnd(eof);
	//iter.data() 	= me_.GetDataPage(iter.page(), iter.key_idx());
}



//PROTECTED API:
M_PARAMS
typename M_TYPE::DataPage* M_TYPE::create_datapage(NodeBase* node, Int idx)
{
	MEMORIA_TRACE(me_, "[node.id, idx]", node->id(), idx);

	//        DataPage *data      = new (me_.allocator()) DataPage;

	DataPage *data      = static_cast<DataPage*>(me_.allocator().CreatePage());
	data->init();

	data->parent_idx()      = idx;
	data->parent_id()       = node->id();

	data->model_hash()      = me_.hash();
	data->page_type_hash()  = DataPage::hash();

	me_.SetLeafData(node, idx, data->id());

	MEMORIA_TRACE(me_, "[data.id]", data->id());

	return data;
}


// ------------------------------------------ PRIVATE API ------------------------------------------
M_PARAMS
void M_TYPE::import_pages(
		Iterator &iter,
		Buffer &block,
		BufferContentDescriptor &descriptor
)
{
	NodeBase *node = iter.page();
	BigInt key_idx = iter.key_idx();
	BigInt data_pos = iter.data_pos();

	DataPage* suffix_data_page;

	BigInt length  = descriptor.length();

	BigInt max_datapage_size = DataPage::get_max_size();
	BigInt target_datapage_capacity = max_datapage_size - iter.data()->data().size();

	// the size of data block that fits into the target data page
	BigInt data_prefix;


	if (data_pos == iter.data()->data().size() || (data_pos == 0 && iter.data()->data().size() == 0))
	{
		data_prefix = target_datapage_capacity >= length ? length : target_datapage_capacity;
		suffix_data_page = iter.GetNextDataPage(iter.page(), iter.data());
	}
	else
	{
		// data_pos == 0
				// Don't import anything into the target datapage
		// Create a new one instead
		data_prefix = 0;
		suffix_data_page = iter.data();
	}

	if (data_prefix > 0)
	{
		// Import size_prefix part of data block into the target datapage
		MEMORIA_TRACE(me_, "data_prefix > 0");
		descriptor.length() = data_prefix;
		import_small_block(node, key_idx, data_pos, block, descriptor);
		key_idx++;
	}

	// key_idx points to the the btree node to start insert into

	// The total number of full datapages to import
	BigInt total_full_datapages = (length - data_prefix) / max_datapage_size;

	// The data block remainder
	BigInt data_suffix = length - data_prefix - total_full_datapages * max_datapage_size;

	BigInt target_indexpage_max_capacity = me_.GetMaxCapacity(node);
	BigInt target_indexpage_capacity = me_.GetCapacity(node);

	MEMORIA_TRACE(me_, "values [total_full_datapages, data_suffix]", total_full_datapages, data_suffix);
	MEMORIA_TRACE(me_, "values [key_idx, target_indexpage_max_capacity, target_indexpage_capacity]", key_idx, target_indexpage_max_capacity, target_indexpage_capacity);

	BigInt index_prefix;

	if (key_idx == target_indexpage_max_capacity)
	{
		// target indexpage is full and we are at it's end
		index_prefix = 0;
		MEMORIA_TRACE(me_, "The target index page is full");
	}
	else if (key_idx == me_.GetChildrenCount(node))
	{
		// target indexpage is not full but we are at it's end
		index_prefix = target_indexpage_capacity >= total_full_datapages ? total_full_datapages : target_indexpage_capacity;
		MEMORIA_TRACE(me_, "End of target indexpage", key_idx, index_prefix);
	}
	else {
		// insert into the body of target indexpage
		if (total_full_datapages > target_indexpage_capacity)
		{
			// split indexpage if it's capacity is not
			// enough to store total_full_datapages data pages
			MEMORIA_TRACE(me_, "split target indexpage");

			me_.SplitBTreeNode(node, key_idx);

			target_indexpage_capacity = me_.GetCapacity(node);
			index_prefix = target_indexpage_capacity >= total_full_datapages ? total_full_datapages : target_indexpage_capacity;
		}
		else if (total_full_datapages > 0)
		{
			// make a room in the target indexpage
			MEMORIA_TRACE(me_, "insert space for index_prefix");

			// Assign increase_children_count = false for InsertSpace() because
			// import_several_pages() increases node children count

			me_.InsertSpace(node, key_idx, total_full_datapages, false);

			index_prefix = total_full_datapages;
		}
		else
		{
			// we don't have any full data pages to insert
			index_prefix = 0;
		}
	}

	descriptor.length() = max_datapage_size;

	if (index_prefix > 0)
	{
		import_several_pages(node, key_idx, block, descriptor, index_prefix);
		key_idx += index_prefix;

		MEMORIA_TRACE(me_, "index_prefix: node size: ", me_.GetChildrenCount(node));
	}

	// FIXME: me_.GetMaxCapacity(node) may be different
	// after several indexpages are inserted into the btree
	// Use the smallest MaxCapacity value for all btree node types.
	BigInt max_indexpage_size = me_.GetMaxCapacity(node);
	BigInt total_full_indexpages = (total_full_datapages - index_prefix) / max_indexpage_size;

	MEMORIA_TRACE(me_, "total pages [total_full_indexpages, index_prefix]", total_full_indexpages, index_prefix);

	// Import several full indexpages into the btree
	for (BigInt c = 0; c < total_full_indexpages; c++)
	{
		node = me_.SplitBTreeNode(node, me_.GetChildrenCount(node), 0);

		import_several_pages(node, 0, block, descriptor, max_indexpage_size);

		key_idx = me_.GetChildrenCount(node);
	}


	BigInt index_suffix = total_full_datapages - index_prefix - total_full_indexpages * max_indexpage_size;

	if (index_suffix > 0)
	{
		NodeBase* suffix_node = iter.GetNextNode(node);
		MEMORIA_TRACE(me_, "index_suffix [index_suffix, suffix_node.id]", index_suffix, suffix_node != NULL ? suffix_node->id() : ID(0));

		if (suffix_node != NULL && me_.GetCapacity(suffix_node) >= index_suffix)
		{
			MEMORIA_TRACE(me_, "use next page", suffix_node->id());
			node = suffix_node;
			me_.InsertSpace(node, 0, index_suffix, false);
		}
		else
		{
			Int split_at = me_.GetChildrenCount(node);
			MEMORIA_TRACE(me_, "split the suffix indexpage at", split_at);

			//FIXME: If me_.GetChildrenCount(node) is valid here?!
			//Note this case in SplitBTreeNode() docs if so
			node = me_.SplitBTreeNode(node, split_at);
		}

		import_several_pages(node, 0, block, descriptor, index_suffix);

		key_idx = index_suffix;
	}



	Int out_pos = 0;
	if (data_suffix > 0)
	{
		if (suffix_data_page != NULL)
		{
			// FIXME: when the suffix_data_page was moved during split/insert
			// it wasn't properly reparented
			//
			node = me_.GetDataParent(suffix_data_page);
			key_idx = suffix_data_page->parent_idx();
			Int suffix_free = max_datapage_size - suffix_data_page->data().size();

			MEMORIA_TRACE(me_, "DATA_SUFFIX page [node.id, suffix_data_page.id, suffix_data_page.parent_id, key_idx, suffix_data.free, node.id, node.capacity]", node->id(), suffix_data_page->id(), suffix_data_page->parent_id(), key_idx, suffix_free, node->id(), me_.GetCapacity(node));

//			for (Int c = 0; c < me_.GetChildrenCount(node); c++)
//			{
//				DataPage* data = me_.GetDataPage(node, c);
//				MEMORIA_TRACE(me_, "c=", c, "pidx=", data->parent_idx(), data->id());
//			}

			if (data_suffix <= suffix_free)
			{
				// import data suffix into suffix_data_page
				out_pos = data_suffix;
			}
			else if (me_.GetCapacity(node) > 0)
			{
				me_.InsertDataPage(node, key_idx);

			}
			else if (key_idx < me_.GetMaxCapacity(node) - 1)
			{
				NodeBase* node_right = me_.SplitBTreeNode(node, key_idx);
				me_.InsertDataPage(node, key_idx);
			}
			else {
				node = me_.SplitBTreeNode(node, key_idx);
				me_.InsertDataPage(node, 0);
				key_idx = 0;
			}
		}
		else {
			// EOF
			Int max_capacity = me_.GetMaxCapacity(node);
			MEMORIA_TRACE(me_, "DATA_SUFFIX EOF [node.id, node.capacity, node.max_capacity, key_idx]", node->id(), me_.GetCapacity(node), max_capacity, key_idx);
			if (key_idx >= max_capacity)
			{
				node = me_.SplitBTreeNode(node, max_capacity);
				me_.InsertDataPage(node, 0);
				key_idx = 0;
			}
			else {
				me_.InsertDataPage(node, key_idx);
			}
		}

		descriptor.length() = data_suffix;
		import_small_block(node, key_idx, 0, block, descriptor);
	}

	if (suffix_data_page != NULL)
	{
		iter.page() 	= me_.GetDataParent(suffix_data_page);
		iter.key_idx() 	= suffix_data_page->parent_idx();
		iter.data() 	= suffix_data_page;
		iter.data_pos() 		= out_pos;
	}
	else {

		iter.page() 	= node;
		iter.key_idx() 	= me_.GetChildrenCount(node) - 1;
		iter.data() 	= me_.GetDataPage(node, iter.key_idx());
		iter.data_pos() 		= iter.data()->data().size();
	}

	//FIXME: this operation is too expensive for small blocks
	iter.Init();
}

M_PARAMS
void M_TYPE::import_several_pages(
		NodeBase *node,
		BigInt key_idx,
		const Buffer &block,
		BufferContentDescriptor &descriptor,
		BigInt page_count)
{
	MEMORIA_TRACE(me_, "[node.id, key_idx, length, page_count]", node->id(), key_idx, page_count);

	BigInt total_indexes[Indexes] = {0};

	for (BigInt c = key_idx; c < key_idx + page_count; c++)
	{
		me_.import_data(
				me_.create_datapage(node, c),
				c,
				0,
				block,
				descriptor
		);

		BigInt* indexes = descriptor.indexes();
		me_.SetKeys(node, c, indexes);

		for (Int d = 0; d < Indexes; d++)
		{
			total_indexes[d] += indexes[d];
		}
	}

	me_.AddChildrenCount(node, page_count);

	me_.Reindex(node);

	NodeBase* parent = me_.GetParent(node);

	MEMORIA_TRACE(me_, "KEYS", total_indexes[0]);
	if (parent != NULL)
	{
		me_.UpdateBTreeKeys(parent, node->parent_idx(), total_indexes, true);
	}

	MEMORIA_TRACE(me_, "DONE");
}



// FIXME: check: make a room for inserted data if necessary
M_PARAMS
void M_TYPE::import_small_block(
		NodeBase *node,
		BigInt idx,
		BigInt pos,
		const Buffer &block,
		BufferContentDescriptor &descriptor
)
{
	MEMORIA_TRACE(me_, "[node.id, idx, pos, descr.length]", node->id(), idx, pos, descriptor.length());

	DataPage* data_page = me_.GetDataPage(node, idx);
	if (data_page == NULL)
	{
		data_page = me_.create_datapage(node, idx);
	}

	me_.import_data(data_page, idx, pos, block, descriptor);

	MEMORIA_TRACE(me_, "KEYS", descriptor.indexes()[0]);

	me_.UpdateBTreeKeys(node, idx, descriptor.indexes(), true); //FIXME: add or replace ?
}


M_PARAMS
void M_TYPE::import_data(
		DataPage *page,
		BigInt idx,
		BigInt pos,
		const Buffer &data,
		BufferContentDescriptor &descriptor)
{
	BigInt start    = descriptor.start();
	BigInt length   = descriptor.length();
	MEMORIA_TRACE(me_, "[data.id, idx, pos, start, length, descr.indexes[0]]", page->id(), idx, pos, start, length, descriptor.indexes()[0]);

	BigInt usage 	= page->data().size();

	if (pos < usage)
	{
		page->data().shift(pos, length);
	}

	MEMORIA_TRACE(me_, "CopyData", page->id(), start, pos, length);
	me_.copy_data(data, page, start, pos, length);

	usage += length;

	page->data().size() = usage;

	descriptor.indexes()[0] = length;

	descriptor.start() += length;
	MEMORIA_TRACE(me_, "Usage", descriptor.indexes()[0]);
}

M_PARAMS
void M_TYPE::move_data_in_page_create(Iterator &iter, BigInt local_idx, CountData &prefix)
{
	MEMORIA_TRACE(me_, "BEGIN1");
	DataPage* to = me_.create_datapage(iter.page(), iter.data()->parent_idx() + 1);
	me_.move_data_in_page(iter.data(), to, local_idx, prefix);
}

M_PARAMS
void M_TYPE::move_data_in_page_create(DataPage *from, NodeBase *node, BigInt idx, BigInt local_idx, CountData &prefix)
{
	MEMORIA_TRACE(me_, "BEGIN2");
	DataPage* to = me_.create_datapage(node, idx);
	//FIXME from-> has incorrect type for this expression
	move_data_in_page(from, to, local_idx, from->
			data()->
			header().get_size(), prefix);
}

M_PARAMS
void M_TYPE::move_data_in_page(Iterator &iter, BigInt local_idx, CountData &prefix)
{
	Int idx = iter.data()->parent_idx() + 1;
	DataPage *to = me_.GetDataPage(iter.page(), idx);
	me_.move_data_in_page(iter.data(), to, local_idx, prefix);
}

M_PARAMS
void M_TYPE::move_data_in_page(DataPage *from, DataPage *to, BigInt local_idx, CountData &prefix)
{
	MEMORIA_TRACE(me_, "BEGIN", from->id(), to->id(), local_idx);

	//TODO: does it better to update indexes and sizes here?
	BigInt keys[Indexes];
	me_.move_data(from, to, local_idx, prefix, keys);

	NodeBase *from_node = me_.GetDataParent(from);
	me_.UpdateBTreeKeys(from_node, from->parent_idx(), keys, true);

	for (Int c = 0; c < Indexes; c++) keys[c] = -keys[c];

	NodeBase *to_node = me_.GetDataParent(to);
	me_.UpdateBTreeKeys(to_node, to->parent_idx(), keys, true);
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
