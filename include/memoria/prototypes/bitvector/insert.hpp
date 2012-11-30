
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


node_base *create_node(txn_t &txn, SmallType level, bool root)
{
    node_base *node;
    if (level == 1)
    {
        if (root)
        {
            root_small_node *_node = new (_allocator, txn) root_small_node;
            _node->set_key_size(KEY_SIZE__SMALL);
            node = _node;
        }
        else
        {
            small_node *_node = new (_allocator, txn) small_node;
            _node->set_key_size(KEY_SIZE__SMALL);
            node = _node;
        }
    }
    else if (level == 2)
    {
        if (root)
        {
            root_medium_node *_node = new (_allocator, txn) root_medium_node;
            _node->set_key_size(KEY_SIZE__MEDIUM);
            node = _node;
        }
        else
        {
            medium_node *_node = new (_allocator, txn) medium_node;
            _node->set_key_size(KEY_SIZE__MEDIUM);
            node = _node;
        }
    }
    else if (level == 3)
    {
        if (root)
        {
            root_large_node *_node = new (_allocator, txn) root_large_node;
            _node->set_key_size(KEY_SIZE__LARGE);
            node = _node;
        }
        else
        {
            large_node *_node = new (_allocator, txn) large_node;
            _node->set_key_size(KEY_SIZE__LARGE);
            node = _node;
        }
    }
    else
    {
        if (root)
        {
            root_huge_node *_node = new (_allocator, txn) root_huge_node;
            _node->set_key_size(KEY_SIZE__HUGE);
            node = _node;
        }
        else
        {
            huge_node *_node = new (_allocator, txn) huge_node;
            _node->set_key_size(KEY_SIZE__HUGE);
            node = _node;
        }
    }

    node->level() = level;
    return node;
}

bitmap_node* create_bitmap(node_base* node, index_t idx, txn_t &txn)
{
    bitmap_node *bitmap = new (allocator(), txn) bitmap_node;

    bitmap->parent_idx() = idx;
    bitmap->parent_id() = node->id();

    BV_NODE_CAST2(node,
                  __node->map().key(0, idx) = 0;
                  __node->map().key(1, idx) = 0;
                  __node->map().key(2, idx) = 0;

                  __node->map().data(idx) = bitmap->id();

                  if (idx >= __node->children_count())
    {
    __node->children_count() += idx - __node->children_count() + 1;
    }
                 );
    return bitmap;
}


void update_btree_parent(node_base *node, csize_t size, csize_t rank, csize_t bit_size, txn_t &txn)
{
    node_base* parent = get_parent(node, txn);
    if (parent != NULL)
    {
        update_btree_nodes(parent, node->parent_idx(), size, rank, bit_size, allocator(), txn);
    }
}


static void update_btree_nodes(node_base *page, index_t idx, csize_t size, csize_t rank, csize_t bit_size, Allocator &_allocator, txn_t &txn)
{
    BV_NODE_CAST2(page,
    {
        __page->map().key(0, idx) += size;
        __page->map().key(1, idx) += rank;
        __page->map().key(2, idx) += bit_size;

        __page->reindex();
    });

    while (!page->is_root())
    {
        SmallType parent_idx = page->parent_idx();
        node_base* parent = static_cast<node_base*>(_allocator.get(txn, page->parent_id()));

        BV_NODE_CAST2(parent,
        {
            __parent->map().key(0, parent_idx) += size;
            __parent->map().key(1, parent_idx) += rank;
            __parent->map().key(2, parent_idx) += bit_size;

            __parent->reindex();
        });

        page = parent;
    }
}

static void update_inserted(node_base *page, index_t idx, bool inserted, bool hasnt_label, Allocator &_allocator, txn_t &txn)
{
    if (inserted)
    {
        page->inserted_idx() = idx;
        page->hasnt_label() = hasnt_label;
    }
    else
    {
        page->inserted_idx() = -1;
        page->hasnt_label() = false;
    }

    while (!page->is_root())
    {
        node_base* parent = static_cast<node_base*>(_allocator.get(txn, page->parent_id()));

        if (inserted)
        {
            parent->inserted_idx() = page->parent_idx();
            parent->hasnt_label() = hasnt_label;
        }
        else
        {
            parent->inserted_idx() = -1;
            parent->hasnt_label() = false;
        }

        page = parent;
    }
}

void insert_space(node_base *node, index_t from, index_t count, txn_t &txn)
{
    BV_NODE_CAST2(node,
        if (__node->inserted_idx() >= 0 && from <= __node->inserted_idx())
        {
            __node->inserted_idx() += count;
        }

        __node->map().move_data(from + count, from, __node->children_count() - from);

        for (index_t c = from; c < from + count; c++)
        {
            for (index_t d = 0; d < __node->INDEXES; d++)
            {
                __node->map().key(d, c) = 0;
            }
            __node->map().data(c) = 0;
        }

        __node->children_count() += count;

        for (index_t c = from + count; c < __node->children_count(); c++)
        {
            tree_node *child = static_cast<tree_node*>(allocator().get(txn, __node->map().data(c)));
            child->parent_idx() += count;
        }
    );
}

void remove_space(node_base *node, index_t from, index_t count, txn_t &txn)
{
    remove_space(node, from, count, false, false, txn);
}

void remove_space(node_base *node, index_t from, index_t count, bool update, bool remove_children, txn_t &txn)
{
    if (update)
    {
        csize_t size = 0, rank = 0, bitsize = 0;
        BV_NODE_CAST2(node,
                      for (index_t c = from; c < from + count; c++)
    {
        size += __node->map().key(0, c);
            rank += __node->map().key(1, c);
            bitsize += __node->map().key(2, c);
        }
                     );

        if (node->inserted_idx() >= from && node->inserted_idx() < from + count)
        {
            update_inserted(node, -1, false, false, _allocator, txn);
        }

        update_btree_parent(node, -size, -rank, -bitsize, txn);
    }

    BV_NODE_CAST2(node, 
        if (__node->is_leaf())
        {
            __node->page_count() -= count;
            for (index_t c = from; c < from + count; c++)
            {
                _allocator.free(txn, __node->map().data(c));
            }
        }
        else
        {
            for (index_t c = from; c < from + count; c++)
            {
                node_base *child = get_child(__node, c, _allocator, txn);
                __node->page_count() -= child->page_count();
                if (remove_children)
                {
                    remove_node(child, txn);
                }
            }
        }

        if (__node->inserted_idx() >= 0)
        {
            if (__node->inserted_idx() >= from + count)
            {
                __node->inserted_idx() -= count;
            }
            else if (__node->inserted_idx() >= from && __node->inserted_idx() < from + count)
            {
                __node->inserted_idx() = -1;
            }
        }

        if (from + count < __node->children_count())
        {
            __node->map().move_data(from, from + count, __node->children_count() - (from + count));
        }

        for (index_t c = __node->children_count() - count; c < __node->children_count(); c++)
        {
            for (index_t d = 0; d < __node->INDEXES; d++)
            {
                __node->map().key(d, c) = 0;
            }
            __node->map().data(c) = 0;
        }
        __node->children_count() -= count;

        for (index_t c = from; c < __node->children_count(); c++)
        {
            tree_node *child = static_cast<tree_node*>(allocator().get(txn, __node->map().data(c)));
            child->parent_idx() -= count;
        }

        __node->map().reindex();
    );
}

void remove_node(node_base *node, txn_t &txn)
{
    if (node->is_leaf())
    {
        BV_NODE_CAST2(node,
            for (index_t c = 0; c < __node->children_count(); c++)
            {
                _allocator.free(txn, __node->map().data(c));
            }
        );
    }
    else
    {
        for (index_t c = 0; c < get_children_count(node); c++)
        {
            node_base *child = get_child(node, c, _allocator, txn);
            remove_node(child, txn);
        }
    }
    _allocator.free(txn, node->id());
}

node_base* split_node(node_base *one, node_base *parent, index_t parent_idx, index_t from, index_t shift, txn_t &txn)
{
    node_base* two = create_node(txn, one->level(), false);
    index_t count;

    BV_NODE_CAST3(one, two,
    {
        __two->level() = __one->level();
        __two->set_leaf(__one->is_leaf());

        count = __one->children_count() - from;

        if (__two->children_count() > 0)
        {
            __two->map().move_data(count + shift, 0, __two->children_count());
        }

        __one->map().copy_data(from, count, __two->map(), shift);

        for (index_t c = from; c < from + count; c++)
        {
            for (index_t d = 0; d < __one->INDEXES; d++)
            {
                __one->map().key(d, c) = 0;
            }
            __one->map().data(c) = 0;
        }

        __one->children_count() -= count;
        __two->children_count() += count + shift;

        for (index_t c = 0; c < shift; c++)
        {
            for (index_t d = 0; d < __two->INDEXES; d++)
            {
                __two->map().key(d, c) = 0;
            }
            __two->map().data(c) = 0;
        }

        __one->reindex();
        __two->reindex();

        index_t page_cnt = 0;
        for (index_t c = shift; c < count + shift; c++)
        {
            tree_node *child = static_cast<tree_node*>(allocator().get(txn, __two->map().data(c)));

            child->parent_id() = __two->id();
            child->parent_idx() -= from;
            child->parent_idx() += shift;

            if (!child->isBitmap())
            {
                page_cnt += 1;
            }
            else
            {
                node_base *node = static_cast<node_base*>(child);
                page_cnt += node->page_count();
            }
        }

        for (index_t c = count + shift; c < __two->children_count(); c++)
        {
            tree_node *child = static_cast<tree_node*>(allocator().get(txn, __two->map().data(c)));
            child->parent_idx() += count + shift;
        }

        __two->page_count() += page_cnt;
        __one->page_count() -= page_cnt;

    });

    two->parent_idx() = parent_idx;
    two->parent_id() = parent->id();

    node_base *one_parent = get_parent(one, txn);

    BV_NODE_CAST2(two,
        csize_t size_sum    = 0;
        csize_t rank_sum    = 0;
        csize_t bitsize_sum = 0;
        for (index_t c = shift; c < count + shift; c++)
        {
            size_sum    += __two->map().key(0, c);
            rank_sum    += __two->map().key(1, c);
            bitsize_sum += __two->map().key(2, c);
        }

        update_btree_nodes(one_parent, one->parent_idx(), -size_sum, -rank_sum, -bitsize_sum, _allocator, txn);
    );

    BV_NODE_CAST2(two,
        BV_NODE_CAST2(parent,
            if (parent_idx == __parent->children_count())
            {
                __parent->children_count()++;
            }

            __parent->map().data(parent_idx) = __two->id();

            update_btree_nodes(__parent, parent_idx, __two->map().max_key(0), __two->map().max_key(1), __two->map().max_key(2), _allocator, txn);
        );
    );

    return two;
}

node_base * split_btree_node(node_base *page, index_t count_leaf, index_t shift, txn_t txn)
{
    node_base *parent = get_parent(page, txn);
    node_base *new_page;

    if (parent != NULL)
    {
        index_t idx_in_parent = page->parent_idx();
        if (get_capacity(parent) == 0)
        {
            index_t parent_idx;

            if (idx_in_parent < get_children_count(parent) - 1)
            {
                parent = split_btree_node(parent, idx_in_parent + 1, 1, txn);
                parent_idx = 0;
            }
            else
            {
                parent = split_btree_node(parent, get_children_count(parent) / 2, 0, txn);
                parent_idx = get_children_count(parent);
            }

            new_page = split_node(page, parent, parent_idx, count_leaf, shift, txn);
        }
        else
        {
            insert_space(parent, idx_in_parent + 1, 1, txn);
            new_page = split_node(page, parent, idx_in_parent + 1, count_leaf, shift, txn);
        }
    }
    else
    {
        node_base* root = get_root(txn);
        node_base* new_root = create_node(txn, root->level() + 1, true);

        copy_root_metadata(root, new_root);

        root = root2node(root);

        new_root->parent_id() = root->parent_id();
        new_root->parent_idx() = 0;
        //TODO register new root in the object repository

        root->parent_id() = new_root->id();
        root->parent_idx() = 0;

        BV_NODE_CAST2(new_root,
        {
            for (index_t d = 0; d < __new_root->INDEXES; d++)
            {
                csize_t key;
                BV_NODE_CAST2(root, key = __root->map().max_key(d));
                __new_root->map().key(d, 0) = key;
            }
            __new_root->map().data(0) = root->id();
            __new_root->map().size( ) = 2;
        });

        _root = new_root->id();

        new_page = split_node(page, new_root, 1, count_leaf, shift, txn);
    }

    return new_page;
}



static void set_keys(node_base *node, index_t idx, csize_t size, csize_t rank, csize_t bitsize)
{
    BV_NODE_CAST2(node,
                  __node->map().key(0, idx) = size;
                  __node->map().key(1, idx) = rank;
                  __node->map().key(2, idx) = bitsize;
                 );
}

template <typename Buffer>
void import_several_pages(node_base *node, index_t &prefix, index_t &offset,
                          index_t idx, const Buffer &block, index_t start, index_t page_count,
                          CountData &base_prefix,
                          int content_type, txn_t &txn)
{

    index_t rank, bitsize;
    index_t page_size = bitmap_node::getMaxSize();

    csize_t total_size = 0, total_rank = 0, total_bitsize = 0;
    for (index_t c = idx; c < idx + page_count; c++, start+=page_size)
    {
        import_data(create_bitmap(node, c, txn), c, block, prefix, offset, rank, bitsize,
                    0, start, page_size,
                    base_prefix,
                    content_type, txn);

        set_keys(node, c, page_size, rank, bitsize);

        total_size      += page_size;
        total_rank      += rank;
        total_bitsize   += bitsize;
    }

    BV_NODE_CAST2(node, __node->reindex());

    update_btree_parent(node, total_size, total_rank, total_bitsize, txn);
}

template <typename Buffer>
void import_small_block(node_base *node, index_t idx, index_t &prefix, index_t &offset,
                        const Buffer &block, index_t pos, index_t start, index_t length, CountData &base_prefix,
                        int content_type, txn_t &txn)
{

    bitmap_node *bitmap = get_bitmap(node, idx, txn);
    if (bitmap == NULL)
    {
        bitmap = create_bitmap(node, idx, txn);
    }

    index_t rank, bitsize;
    import_data(bitmap, idx, block, prefix, offset, rank, bitsize, pos, start, length,
                base_prefix, content_type, txn);

    update_btree_nodes(node, idx, length, rank, bitsize, _allocator, txn);
}


template <typename Buffer>
void import_data(bitmap_node *page, index_t idx, const Buffer &data,
                 index_t &prefix, index_t &offset,
                 index_t &rank, index_t &bitsize,
                 index_t pos, index_t start, index_t length,
                 CountData &base_prefix,
                 int content_type, txn_t &txn)
{

    index_t usage = page->header().get_size();

    index_t offset0 = offset - start;
    compute_rank_and_size_up_to(data, prefix, offset, rank, bitsize, start + length, get_node_bits(), get_data_bits());

    if (pos < usage)
    {
        page->shift(pos, length);
    }

    if (page->header().get_inserted() >= pos)
    {
        page->header().set_inserted(page->header().get_inserted() + length);
    }

    copy_bits(data, page->bits(), start, pos, length);

    usage += length;

    page->header().set_size(usage);

    index_t offSet1 = page->header().get_offset();

    if (usage == length)
    {
        page->header().set_offset(offset0 >= usage ? usage : offset0);
    }
    else if (pos <= offSet1)
    {
        if (offset0 < length)
        {
            page->header().set_offset(pos + offset0 < usage ? pos + offset0 : usage);
        }
        else
        {
            page->header().set_offset(offSet1 + length < usage ? offSet1 + length : usage);
        }
    }

    if (content_type == BIT_FOR_PARENT)
    {
        page->header().set_inserted(pos);
        page->header().set_hasnt_label(true);
        _marked_page_id = page->id();

        node_base* parent = get_parent(page, txn);
        update_inserted(parent, page->parent_idx(), true, true, _allocator, txn);
    }
    else if (content_type == LABEL)
    {
        bitmap_node *marked_page = get_bitmap(_marked_page_id, txn);
        if (marked_page->header().hasnt_label())
        {
            marked_page->header().set_hasnt_label(false);
            node_base* parent = get_parent(marked_page, txn);
            update_inserted(parent, marked_page->parent_idx(), true, false, _allocator, txn);
        }
    }
    else if (!_marked_page_id.isNull())
    {
        bitmap_node *marked_page = get_bitmap(_marked_page_id, txn);
        marked_page->header().set_inserted(-1);
        marked_page->header().set_hasnt_label(false);

        node_base* parent = get_parent(marked_page, txn);
        update_inserted(parent, -1, false, false, _allocator, txn);
        _marked_page_id.clear();
    }

    csize_t page_offset = page->header().get_offset();
    page->reindex(base_prefix.inserted, base_prefix.hasnt_label, base_prefix.count, page_offset, get_node_bits(), get_data_bits());

    if (page->header().get_inserted() >= 0)
    {
        base_prefix.inserted = true;
        base_prefix.hasnt_label = page->header().hasnt_label();
    }
}


template <typename Buffer>
void import_pages(node_base *node, index_t idx, index_t pos, index_t offset, Buffer &block, index_t start,
                  index_t length, node_base **page0, index_t &idx0, index_t &offset0,
                  CountData &base_prefix, int content_type, txn_t &txn)
{

    index_t page_size = bitmap_node::getMaxSize();
    index_t fp_capacity = page_size - get_key(node, 0, idx);

    index_t prefix;

    if (pos == page_size)
    {
        prefix = fp_capacity >= length ? length : fp_capacity;
        pos = 0;
        if (idx < get_children_count(node) - 1)
        {
            if (get_capacity(node) > 0)
            {
                insert_space(node, idx + 1, 1, txn);
                create_bitmap(node, idx + 1, txn);
            }
            else
            {
                split_btree_node(node, idx + 1, 0, txn);
            }
            idx++;
        }
        else if (idx < get_max_capacity(node) - 1)
        {
            idx++;
        }
        else
        {
            node = split_btree_node(node, idx, 0, txn);
            idx = 0;
        }
    }
    else if (pos == 0)
    {
        prefix = 0;
    }
    else
    {
        prefix = fp_capacity >= length ? length : fp_capacity;
    }

    index_t bit_prefix = 0;//, rank, bitsize;

    if (prefix > 0)
    {
        import_small_block(node, idx, bit_prefix, offset, block, pos, start, prefix, base_prefix, content_type, txn);

        *page0  = node;
        idx0   = idx;
        offset0 = pos;

        start += prefix;
        idx++;
    }

    index_t total_npages = (length - prefix) / page_size;
    index_t suffix = length - prefix - total_npages * page_size;

    index_t index_page_size = get_max_capacity(node);
    index_t fpi_capacity = get_capacity(node);
    //index_t fpi_usage = index_page_size - fpi_capacity;

    index_t index_prefix;

    if (idx == index_page_size)
    {
        index_prefix = 0;
    }
    else if (idx == get_children_count(node))
    {
        index_prefix = fpi_capacity >= total_npages ? total_npages : fpi_capacity ;
    }
    else if (total_npages > fpi_capacity)
    {
        split_btree_node(node, idx, 0, txn);

        fpi_capacity = get_capacity(node);
        index_prefix = fpi_capacity >= total_npages ? total_npages : fpi_capacity;
    }
    else if (total_npages > 0)
    {
        insert_space(node, idx, total_npages, txn);
        index_prefix = total_npages;
    }
    else
    {
        index_prefix = total_npages;
    }

    index_t total_index_pages = (total_npages - index_prefix) / index_page_size;

    if (index_prefix > 0)
    {
        import_several_pages(node, bit_prefix, offset, idx, block, start, index_prefix, base_prefix, content_type, txn);

        if (prefix == 0)
        {
            *page0  = node;
            idx0   = idx;
            offset0 = 0;
        }

        start += index_prefix * page_size;
        idx += index_prefix;
    }

    if (prefix == 0 && index_prefix == 0 && total_index_pages > 0)
    {
        *page0  = node;
        idx0   = idx;
        offset0 = 0;
    }

    index_t c;
    for (c = 0; c < total_index_pages; c++)
    {
        node = split_btree_node(node, get_children_count(node), 0, txn);
        import_several_pages(node, bit_prefix, offset, 0, block, start, index_page_size, base_prefix, content_type, txn);

        start += index_page_size * page_size;
        idx = get_max_capacity(node);
    }

    index_t index_suffix = total_npages - index_prefix - total_index_pages * index_page_size;

    if (index_suffix > 0)
    {
        node_base *next_page = get_next_leaf(node, txn);
        if (next_page != NULL && get_capacity(next_page) >= index_prefix)
        {
            node = next_page;
        }
        else
        {
            idx = 0;
            node = split_btree_node(node, get_children_count(node), 0, txn); //TODO ???? why +1 ?
        }

        if (prefix == 0 && index_prefix == 0 && total_index_pages == 0)
        {
            *page0  = node;
            idx0   = idx;
            offset0 = 0;
        }

        import_several_pages(node, bit_prefix, offset, idx, block, start, index_suffix, base_prefix, content_type, txn);
        start += index_suffix * page_size;
        idx = index_suffix; // may be += ???????
    }

    if (suffix > 0)
    {
        index_t lp_usage = get_key(node, 0, idx);

        if (suffix <= page_size - lp_usage)
        {
            // do nothing there
        }
        else if (get_capacity(node) > 0)
        {
            insert_space(node, idx, 1, txn);
            create_bitmap(node, idx, txn);
        }
        else
        {
            split_btree_node(node, idx, 0, txn);
        }

        if (prefix == 0 && index_prefix == 0 && total_index_pages == 0 && index_suffix == 0)
        {
            *page0  = node;
            idx0   = idx;
            offset0 = 0;
        }

        import_small_block(node, idx, bit_prefix, offset, block, 0, start, suffix, base_prefix, content_type, txn);
    }
}

void move_data_in_page_create(iterator<> &iter, index_t local_idx, CountData &prefix, txn_t &txn)
{
    bitmap_node* to = create_bitmap(iter.page(), iter.data()->parent_idx() + 1, txn);
    move_data_in_page(iter.data(), to, local_idx, iter.data()->header().get_size(), prefix, txn);
}

void move_data_in_page_create(bitmap_node *from, node_base *node, index_t idx, index_t local_idx, CountData &prefix, txn_t &txn)
{
    bitmap_node* to = create_bitmap(node, idx, txn);
    move_data_in_page(from, to, local_idx, from->data()->header().get_size(), prefix, txn);
}

void move_data_in_page(iterator<> &iter, index_t local_idx, CountData &prefix, txn_t &txn)
{
    bitmap_node *to = get_bitmap(iter.page(), iter.data()->parent_idx() + 1, txn);
    move_data_in_page(iter.data(), to, local_idx, iter.data()->header().get_size(), prefix, txn);
}

void move_data_in_page(bitmap_node *from, bitmap_node *to, index_t local_idx, index_t limit, CountData &prefix, txn_t &txn)
{
    iterator<> iter(_allocator, txn, from, 0);

    if (from->header().get_offset() == 0)
    {
        prefix = iter.get_base_prefix();
    }
    else
    {
        prefix.count = 0;
        prefix.inserted = false;
        prefix.hasnt_label = false;
    }

    index_t from_size0 = from->header().get_size();
    index_t from_rank0 = from->header().get_rank();
    index_t from_bitsize0 = from->header().getBitsize();

    from->move_data(*to, prefix, local_idx, limit, get_node_bits(), get_data_bits());

    index_t from_size1 = from->header().get_size();
    index_t from_rank1 = from->header().get_rank();
    index_t from_bitsize1 = from->header().getBitsize();

    index_t size = from_size1 - from_size0;
    index_t rank = from_rank1 - from_rank0;
    index_t bitsize = from_bitsize1 - from_bitsize0;

    node_base *from_node = get_parent(from, txn);
    update_btree_nodes(from_node, from->parent_idx(), size, rank, bitsize, _allocator, txn);

    node_base *to_node = get_parent(to, txn);
    update_btree_nodes(to_node, to->parent_idx(), -size, -rank, -bitsize, _allocator, txn);
}

template <typename Buffer>
void insert_bitmap_block(iterator<> &iter, Buffer &block, index_t offset, index_t start, index_t length, int content_type)
{
    txn_t &txn = iter.txn();

    node_base *page = iter.page();
    index_t prefix = 0;

    node_base*  __index =  iter.page();
    index_t     __idx     = __index != NULL ? __index->parent_idx() : -1;
    index_t     __offset  = iter.idx();

    index_t page_size = bitmap_node::getMaxSize();

    if (iter.isEmpty())
    {
        if (_root.isNull())
        {
            page = create_node(txn, 1, true);
            page->set_leaf(true);
            _root = page->id();
            __index = page;
        }

        CountData zero(0, false, false);
        import_pages(page, 0, 0, offset, block, start, length, &__index, __idx, __offset, zero, content_type, txn);

        iter.resetState();

        iter.setStart(true);
        iter.idx() = 0;
        iter.data() = get_bitmap(page, 0, txn);
    }
    else
    {
        index_t bit_idx = iter.idx();
        index_t usage = get_key(page, 0, iter.data()->parent_idx());
        CountData base_prefix;

        if (usage + length <= page_size)
        {
            base_prefix = iter.get_base_prefix();

            import_small_block(iter.page(), iter.data()->parent_idx(), prefix, offset, block, bit_idx, start, length, base_prefix, content_type, txn);

            __index = iter.page();
            __idx = iter.data()->parent_idx();
            __offset = bit_idx;
        }
        else if (!iter.isEof())
        {
            if (bit_idx > 0)
            {
                index_t idx = iter.data()->parent_idx();
                if (idx < get_children_count(page) - 1)
                {
                    if (page_size - get_key(page, 0, idx + 1) >= usage - bit_idx)
                    {
                        move_data_in_page(iter, bit_idx, base_prefix, txn);
                    }
                    else if (get_capacity(page) > 0)
                    {
                        insert_space(page, idx + 1, 1, txn);
                        create_bitmap(page, idx + 1, txn);
                        move_data_in_page(iter, bit_idx, base_prefix, txn);

                        __idx = idx;
                        __index = page;
                    }
                    else
                    {
                        split_btree_node(page, idx + 1, 0, txn);
                        move_data_in_page_create(iter, bit_idx, base_prefix, txn);
                    }
                }
                else if (idx == get_max_capacity(page) - 1)
                {
                    page = split_btree_node(page, idx, 0, txn);

                    iter.idx() = 0;
                    iter.page() = page;

                    move_data_in_page(iter, bit_idx, base_prefix, txn);
                }
                else
                {
                    move_data_in_page(iter, bit_idx, base_prefix, txn);
                }
            }
            else
            {
                base_prefix = iter.get_base_prefix();
            }

            import_pages(iter.page(), iter.data()->parent_idx(), iter.idx(), offset, block, start, length, &__index, __idx, __offset, base_prefix, content_type, txn);
        }
        else
        {
            base_prefix = iter.get_base_prefix();
            import_pages(iter.page(), iter.data()->parent_idx(), iter.idx(), offset, block, start, length, &__index, __idx, __offset, base_prefix, content_type, txn);
        }
    }

    iter.setEof(false);
    iter.page() = __index;
    iter.idx()  = __offset;
    iter.data() = get_bitmap(__index, __idx, txn);
}
