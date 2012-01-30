
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
    typedef typename Types::Buffer                                         	 	Buffer;
    typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
    typedef typename Types::CountData                                       	CountData;


    static const Int Indexes                                                    = Types::Indexes;


//PUBLIC API:

    void InsertDataBlock(Iterator &iter, Buffer &block, BufferContentDescriptor &descriptor);

    int datapages;

//PROTECTED API:
    DataPageG create_datapage(NodeBaseG& node, Int idx);


private:

    void import_pages(
    		Iterator &iter,
    		Buffer &block,
    		BufferContentDescriptor &descriptor
    );

    void import_several_pages(
            NodeBaseG& node,
            BigInt key_idx,
            const Buffer &block,
            BufferContentDescriptor &descriptor,
            BigInt page_count);


    // FIXME: check: make a room for inserted data if necessary
    void import_small_block(
            NodeBaseG& node,
            BigInt idx,
            BigInt pos,
            const Buffer &block,
            BufferContentDescriptor &descriptor
    );


    void import_data(
                DataPageG& page,
                BigInt idx,
                BigInt pos,
                const Buffer &data,
                BufferContentDescriptor &descriptor);

    void move_data_in_page_create(Iterator &iter, BigInt local_idx, CountData &prefix);

    void move_data_in_page_create(DataPageG& from, NodeBaseG& node, BigInt idx, BigInt local_idx, CountData &prefix);

    void move_data_in_page(Iterator &iter, BigInt local_idx, CountData &prefix);

    void move_data_in_page(DataPageG& from, DataPageG& to, BigInt local_idx, CountData &prefix);

MEMORIA_CONTAINER_PART_END

#define M_TYPE 		MEMORIA_CONTAINER_TYPE(memoria::dynvector::InsertName)
#define M_PARAMS 	MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::InsertDataBlock(Iterator &iter, Buffer &block, BufferContentDescriptor &descriptor)
{
	NodeBaseG& 	page 				= iter.page();
	BigInt 		max_datapage_size 	= DataPage::get_max_size();

	MEMORIA_TRACE(me(), "iterator is not empty", block.size(), page, me()->root());

	BigInt& data_idx = iter.data_pos();

	if (iter.IsEmpty())
	{
		iter.key_idx() 	= 0;
		iter.data_pos()	= 0;
		iter.data() 	= me()->InsertDataPage(page, iter.key_idx());
	}

	//BigInt usage = iter.data() != NULL ? me()->GetKey(page, 0, iter.data()->parent_idx()) : 0;
	BigInt usage = iter.data() != NULL ? iter.data()->data().size() : 0;

	if (usage + descriptor.length() <= max_datapage_size)
	{
		// The target datapage has enough free space to inset to
		import_small_block(iter.page(), iter.data()->parent_idx(), data_idx, block, descriptor);
		data_idx += descriptor.length();
		me()->datapages = 0;
	}
	else if (!iter.IsEof())
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

			DataPageG next_data = iter.GetNextDataPage(page, iter.data());

			if (next_data != NULL)
			{
				BigInt next_data_free_space = max_datapage_size - next_data->data().size();
				BigInt move_amount = usage - data_idx;

				MEMORIA_TRACE(me(), "NextData is not NULL. page.capacity", me()->GetCapacity(page), iter.key_idx());

				if (next_data_free_space < move_amount)
				{
					// There is no enough room in the next datapage
					// Create a new datapage to move data into
					// Split btree if nesessary

					// TODO: Implement as split_datapage()/merge_datapage() methods

					BigInt next_key_idx = iter.key_idx() + 1;

					NodeBaseG next_page = page;
					if (me()->GetCapacity(page) == 0)
					{
						if (next_key_idx < me()->GetMaxCapacity(page))
						{
							me()->SplitBTreeNode(next_page, next_key_idx);
						}
						else {
							next_page = me()->SplitBTreeNode(next_page, next_key_idx);
							next_key_idx = 0;
						}
					}

					next_data = me()->InsertDataPage(next_page, next_key_idx);
				}
			}
			else if (me()->GetCapacity(page) > 0)
			{
				MEMORIA_TRACE(me(), "NextData is NULL");
				next_data = me()->InsertDataPage(page, iter.key_idx() + 1);
			}
			else
			{
				NodeBaseG new_node = me()->SplitBTreeNode(page, iter.key_idx() + 1);
				next_data = me()->InsertDataPage(new_node, 0);
			}

			//FIXME: move base_prefix to the iterator?
			CountData &base_prefix = descriptor.base_prefix();
			me()->move_data_in_page(iter.data(), next_data, data_idx, base_prefix);
		}

		import_pages(iter, block, descriptor);
	}
	else
	{
		import_pages(iter, block, descriptor);
	}


//	iter.SetStart(false);
//	iter.SetEnd(eof);
	//iter.data() 	= me()->GetDataPage(iter.page(), iter.key_idx());
}



//PROTECTED API:
M_PARAMS
typename M_TYPE::DataPageG M_TYPE::create_datapage(NodeBaseG& node, Int idx)
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


// ------------------------------------------ PRIVATE API ------------------------------------------
M_PARAMS
void M_TYPE::import_pages(
		Iterator &iter,
		Buffer &block,
		BufferContentDescriptor &descriptor
)
{
	if (iter.data()->data().size() > 0 && iter.data_pos() == 0)
	{
		if (iter.PrevKey())
		{
			iter.data() 	= me()->GetDataPage(iter.page(), iter.key_idx(), Allocator::READ);
			iter.data_pos() = iter.data()->data().size();
		}
		else {
			me()->InsertDataPage(iter.page(), 0);

			iter.data()		= me()->GetDataPage(iter.page(), 0, Allocator::READ);
			iter.data_pos() = 0;
			iter.key_idx()	= 0;
		}
	}



	NodeBaseG node = iter.page();
	BigInt key_idx = iter.key_idx();
	BigInt data_pos = iter.data_pos();

	DataPageG suffix_data_page;

	BigInt length  = descriptor.length();

	BigInt max_datapage_size = DataPage::get_max_size();
	BigInt target_datapage_capacity = max_datapage_size - iter.data()->data().size();

	// the size of data block that fits into the target data page
	BigInt data_prefix = target_datapage_capacity >= length ? length : target_datapage_capacity;

	suffix_data_page = iter.GetNextDataPage(iter.page(), iter.data());

	if (data_prefix > 0)
	{
		// Import size_prefix part of data block into the target datapage
		descriptor.length() = data_prefix;
		import_small_block(node, key_idx, data_pos, block, descriptor);
	}

	key_idx++;

	// key_idx points to the the btree node to start insert into

	// The total number of full datapages to import
	BigInt total_full_datapages = (length - data_prefix) / max_datapage_size;

	// The data block remainder
	BigInt data_suffix = length - data_prefix - total_full_datapages * max_datapage_size;

	BigInt target_indexpage_max_capacity = me()->GetMaxCapacity(node);
	BigInt target_indexpage_capacity = me()->GetCapacity(node);

	BigInt index_prefix;

	if (key_idx == target_indexpage_max_capacity)
	{
		// target indexpage is full and we are at it's end
		index_prefix = 0;
	}
	else if (key_idx == me()->GetChildrenCount(node))
	{
		// target indexpage is not full but we are at it's end
		index_prefix = target_indexpage_capacity >= total_full_datapages ? total_full_datapages : target_indexpage_capacity;
	}
	else {
		// insert into the body of target indexpage
		if (total_full_datapages > target_indexpage_capacity)
		{
			// split indexpage if it's capacity is not
			// enough to store total_full_datapages data pages

			me()->SplitBTreeNode(node, key_idx);

			target_indexpage_capacity = me()->GetCapacity(node);
			index_prefix = target_indexpage_capacity >= total_full_datapages ? total_full_datapages : target_indexpage_capacity;
		}
		else if (total_full_datapages > 0)
		{
			// make a room in the target indexpage
			// Assign increase_children_count = false for InsertSpace() because
			// import_several_pages() increases node children count

			me()->InsertSpace(node, key_idx, total_full_datapages, false);

			index_prefix = total_full_datapages;
		}
		else
		{
			// we don't have any full data pages to insert
			index_prefix = 0;
		}
	}

	descriptor.length() = max_datapage_size; //What is this????

	if (index_prefix > 0)
	{
		import_several_pages(node, key_idx, block, descriptor, index_prefix);
		key_idx += index_prefix;
	}

	// FIXME: me()->GetMaxCapacity(node) may be different
	// after several indexpages are inserted into the btree
	// Use the smallest MaxCapacity value for all btree node types.
	BigInt max_indexpage_size = me()->GetMaxCapacity(node);
	BigInt total_full_indexpages = (total_full_datapages - index_prefix) / max_indexpage_size;

	// Import several full indexpages into the btree
	for (BigInt c = 0; c < total_full_indexpages; c++)
	{
		node = me()->SplitBTreeNode(node, me()->GetChildrenCount(node), 0);

		import_several_pages(node, 0, block, descriptor, max_indexpage_size);

		key_idx = me()->GetChildrenCount(node);
	}


	BigInt index_suffix = total_full_datapages - index_prefix - total_full_indexpages * max_indexpage_size;

	if (index_suffix > 0)
	{
		NodeBaseG suffix_node = iter.GetNextNode(node);

		if (suffix_node != NULL && me()->GetCapacity(suffix_node) >= index_suffix)
		{
			MEMORIA_TRACE(me(), "use next page", suffix_node->id());
			node = suffix_node;
			me()->InsertSpace(node, 0, index_suffix, false);
		}
		else
		{
			Int split_at = me()->GetChildrenCount(node);

			//FIXME: If me()->GetChildrenCount(node) is valid here?!
			//Note this case in SplitBTreeNode() docs if so
			node = me()->SplitBTreeNode(node, split_at);
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
			node = me()->GetDataParent(suffix_data_page, Allocator::UPDATE);
			key_idx = suffix_data_page->parent_idx();
			Int suffix_free = max_datapage_size - suffix_data_page->data().size();

			if (data_suffix <= suffix_free)
			{
				// import data suffix into suffix_data_page
				out_pos = data_suffix;
			}
			else if (me()->GetCapacity(node) > 0)
			{
				me()->InsertDataPage(node, key_idx);

			}
			else if (key_idx < me()->GetMaxCapacity(node) - 1)
			{
				NodeBaseG node_right = me()->SplitBTreeNode(node, key_idx);
				me()->InsertDataPage(node, key_idx);
			}
			else {
				node = me()->SplitBTreeNode(node, key_idx);
				me()->InsertDataPage(node, 0);
				key_idx = 0;
			}
		}
		else {
			// EOF
			Int max_capacity = me()->GetMaxCapacity(node);

			if (key_idx >= max_capacity)
			{
				node = me()->SplitBTreeNode(node, max_capacity);
				me()->InsertDataPage(node, key_idx - max_capacity);
				key_idx = key_idx - max_capacity;
			}
			else {
				me()->InsertDataPage(node, key_idx);
			}
		}

		descriptor.length() = data_suffix;
		import_small_block(node, key_idx, 0, block, descriptor);
	}

	if (suffix_data_page != NULL)
	{
		iter.page() 	= me()->GetDataParent(suffix_data_page, Allocator::READ);
		iter.key_idx() 	= suffix_data_page->parent_idx();
		iter.data() 	= suffix_data_page;
		iter.data_pos() = out_pos;
	}
	else {

		iter.page() 	= node;
		iter.key_idx() 	= me()->GetChildrenCount(node) - 1;
		iter.data() 	= me()->GetDataPage(node, iter.key_idx(), Allocator::READ);
		iter.data_pos() = iter.data()->data().size();
	}

	//FIXME: this operation is too expensive for small blocks
	iter.Init();
}

M_PARAMS
void M_TYPE::import_several_pages(
		NodeBaseG& node,
		BigInt key_idx,
		const Buffer &block,
		BufferContentDescriptor &descriptor,
		BigInt page_count)
{
	BigInt total_indexes[Indexes] = {0};

	for (BigInt c = key_idx; c < key_idx + page_count; c++)
	{
		DataPageG data = me()->create_datapage(node, c);
		me()->import_data(
				data,
				c,
				0,
				block,
				descriptor
		);

		BigInt* indexes = descriptor.indexes();
		me()->SetKeys(node, c, indexes);

		for (Int d = 0; d < Indexes; d++)
		{
			total_indexes[d] += indexes[d];
		}
	}

	me()->AddChildrenCount(node, page_count);

	me()->Reindex(node);

	NodeBaseG parent = me()->GetParent(node, Allocator::UPDATE);

	if (parent != NULL)
	{
		me()->UpdateBTreeKeys(parent, node->parent_idx(), total_indexes, true);
	}
}



// FIXME: check: make a room for inserted data if necessary
M_PARAMS
void M_TYPE::import_small_block(
		NodeBaseG& node,
		BigInt idx,
		BigInt pos,
		const Buffer &block,
		BufferContentDescriptor &descriptor
)
{
	DataPageG data_page = me()->GetDataPage(node, idx, Allocator::UPDATE);
	if (data_page == NULL)
	{
		data_page = me()->create_datapage(node, idx);
	}

	me()->import_data(data_page, idx, pos, block, descriptor);

	me()->UpdateBTreeKeys(node, idx, descriptor.indexes(), true); //FIXME: add or replace ?
}


M_PARAMS
void M_TYPE::import_data(
		DataPageG& page,
		BigInt idx,
		BigInt pos,
		const Buffer &data,
		BufferContentDescriptor &descriptor)
{
	BigInt start    = descriptor.start();
	BigInt length   = descriptor.length();

	BigInt usage 	= page->data().size();

	if (pos < usage)
	{
		page->data().shift(pos, length);
	}

	me()->copy_data(data, page, start, pos, length);

	usage += length;

	page->data().size() = usage;

	descriptor.indexes()[0] = length;

	descriptor.start() += length;
}

//M_PARAMS
//void M_TYPE::move_data_in_page_create(Iterator &iter, BigInt local_idx, CountData &prefix)
//{
//	DataPageG to = me()->create_datapage(iter.page(), iter.data()->parent_idx() + 1);
//	me()->move_data_in_page(iter.data(), to, local_idx, prefix);
//}

//M_PARAMS
//void M_TYPE::move_data_in_page_create(DataPageG& from, NodeBaseG& node, BigInt idx, BigInt local_idx, CountData &prefix)
//{
//	DataPageG to = me()->create_datapage(node, idx);
//	//FIXME from-> has incorrect type for this expression
//	move_data_in_page(from, to, local_idx, from->data().header().get_size(), prefix);
//}

M_PARAMS
void M_TYPE::move_data_in_page(Iterator &iter, BigInt local_idx, CountData &prefix)
{
	Int idx = iter.data()->parent_idx() + 1;
	DataPageG to = me()->GetDataPage(iter.page(), idx, Allocator::UPDATE);
	me()->move_data_in_page(iter.data(), to, local_idx, prefix);
}

M_PARAMS
void M_TYPE::move_data_in_page(DataPageG& from, DataPageG& to, BigInt local_idx, CountData &prefix)
{
	//TODO: does it better to update indexes and sizes here?
	BigInt keys[Indexes];
	me()->move_data(from, to, local_idx, prefix, keys);

	NodeBaseG from_node = me()->GetDataParent(from, Allocator::UPDATE);
	me()->UpdateBTreeKeys(from_node, from->parent_idx(), keys, true);

	for (Int c = 0; c < Indexes; c++) keys[c] = -keys[c];

	NodeBaseG to_node = me()->GetDataParent(to, Allocator::UPDATE);
	me()->UpdateBTreeKeys(to_node, to->parent_idx(), keys, true);
}

#undef M_TYPE
#undef M_PARAMS

}


#endif
