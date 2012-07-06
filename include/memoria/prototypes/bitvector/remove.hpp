
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



bool can_merge(node_base *page1, node_base *page2)
{
    BV_NODE_CAST2(page2,
        return __page2->children_count() <= get_capacity(page1)
    );
}

static bool is_same_parent(node_base *page1, node_base *page2)
{
    return page1->parent_id() == page2->parent_id();
}

void merge_nodes(node_base *page1, node_base *page2, txn_t txn, bool fix_parent = true)
{
    BV_NODE_CAST3(page1, page2, {
        index_t start = __page1->children_count();

        __page2->map().copy_data(0, __page2->children_count(), __page1->map(), __page1->children_count());

        __page1->children_count() += __page2->children_count();
        __page1->page_count() += __page2->page_count();

        __page1->map().reindex();

        for (index_t c = start; c < __page1->children_count(); c++)
        {
            tree_node *child = static_cast<tree_node*>(allocator().get(txn, __page1->map().data(c)));
            child->parent_id() = __page1->id();
            child->parent_idx() += start;
        }

        if (fix_parent)
        {
            node_base *parent = get_parent(page1, txn);
            index_t parent_idx = __page2->parent_idx();

            BV_NODE_CAST2(parent,
                for (index_t d = 0; d < __parent->INDEXES; d++)
                {
                    __parent->map().key(d, parent_idx - 1) += __parent->map().key(d, parent_idx);
                }
            );

            remove_space(parent, parent_idx, 1, txn);

            BV_NODE_CAST2(parent,__parent->map().reindex());
        }
    });

    _allocator.free(txn, page2);
}

bool merge_btree_nodes(node_base *page1, node_base *page2, txn_t txn)
{
    if (can_merge(page1, page2))
    {
        if (is_same_parent(page1, page2))
        {
            merge_nodes(page1, page2, txn);

            node_base *parent = get_parent(page1, txn);
            if (parent->is_root() && get_children_count(parent) == 1 && can_convert_to_root(page1))
            {

                page1 = node2root(page1);

                copy_root_metadata(parent, page1);

                _root = page1->id();
                page1->parent_idx() = 0;

                _allocator.free(txn, parent);
            }

            return true;
        }
        else
        {
            node_base *parent1 = get_parent(page1, txn);
            node_base *parent2 = get_parent(page2, txn);
            if (merge_btree_nodes(parent1, parent2, txn))
            {
                merge_nodes(page1, page2, txn);
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}

void remove_pages(node_base *start, index_t start_idx, node_base *stop, index_t stop_idx, txn_t &txn)
{
    if (start == NULL || stop == NULL)
    {
        return;
    }
    else if (start->id() == stop->id())
    {
        index_t children_count = get_children_count(start);

        if (start_idx == -1 && stop_idx == children_count)
        {
            while (!start->is_root())
            {
                node_base *parent = get_parent(start, _allocator, txn);
                if (get_children_count(parent) > 1)
                {
                    remove_space(parent, start->parent_idx(), 1, true, true, txn);
                    break;
                }
                else
                {
                    start = parent;
                }
            }

            if (start->is_root())
            {
                remove_node(start, txn);
                _root.clear();
            }
        }
        else if (stop_idx - start_idx > 1)
        {
            remove_space(start, start_idx + 1, stop_idx - start_idx - 1, true, true, txn);

            if (start_idx >= 0 && stop_idx < children_count)
            {
                while (!start->is_leaf())
                {
                    node_base* child0 = get_child(start, start_idx, _allocator, txn);
                    node_base* child1 = get_child(start, start_idx + 1, _allocator, txn);

                    if (can_merge(child0, child1))
                    {
                        index_t child0_size = get_children_count(child0);
                        merge_nodes(child0, child1, txn);

                        if (start->is_root() && get_children_count(start) == 1 && can_convert_to_root(child0))
                        {

                            child0 = node2root(child0);

                            copy_root_metadata(start, child0);

                            _root = child0->id();
                            child0->parent_idx() = 0;

                            _allocator.free(txn, start);
                        }

                        start = child0;
                        start_idx = child0_size - 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        node_base *start_parent  = get_parent(start, _allocator, txn);
        node_base *stop_parent   = get_parent(stop,  _allocator, txn);
        index_t start_parent_idx = start->parent_idx();
        index_t stop_parent_idx  = stop->parent_idx();

        if (start_idx >= 0)
        {
            remove_space(start, start_idx + 1, get_children_count(start) - start_idx - 1, true, true, txn);
        }
        else
        {
            start_parent_idx--;
        }

        if (stop_idx < get_children_count(stop))
        {
            remove_space(stop, 0, stop_idx, true, true, txn);
        }
        else
        {
            stop_parent_idx++;
        }

        remove_pages(start_parent, start_parent_idx, stop_parent, stop_parent_idx, txn);
    }
}

void get_start_page(iterator<> &from, node_base* &start, index_t &start_idx, txn_t &txn)
{
    if (from.prev_bitmap())
    {
        start = get_parent(from.data(), txn);
        start_idx = from.data()->parent_idx();
    }
    else
    {
        start = get_parent(from.data(), txn);
        start_idx = -1;
    }
}

void remove_data(bitmap_node *page,
                 index_t pos, index_t length,
                 CountData &base_prefix,
                 int content_type, txn_t &txn)
{

    index_t usage = page->header().get_size();

    index_t offset1 = page->header().get_offset();

    if (offset1 >= pos + length)
    {
        offset1 -= length;
    }
    else if (offset1 >= pos && offset1 < pos + length)
    {
        CountData tmp = base_prefix;
        offset1 = page->get_offset_for(pos + length, tmp, get_node_bits(), get_data_bits()) - length;
    }

    page->header().set_offset(offset1);

    if (pos + length < usage)
    {
        page->shift(pos + length, -length);
    }

    usage -= length;
    page->header().set_size(usage);

    if (content_type == BIT_FOR_PARENT)
    {
        page->header().set_inserted(-1);
        page->header().set_hasnt_label(false);

        node_base* parent = get_parent(page, txn);
        update_inserted(parent, page->parent_idx(), false, false, _allocator, txn);

        _marked_page_id.clear();
    }
    else if (content_type == LABEL)
    {
        bitmap_node *marked_page = get_bitmap(_marked_page_id, txn);
        if (marked_page->header().hasnt_label())
        {
            marked_page->header().set_hasnt_label(true);
            node_base* parent = get_parent(marked_page, txn);
            update_inserted(parent, marked_page->parent_idx(), true, true, _allocator, txn);
        }
    }
    else if (!_marked_page_id.isNull())
    {
        bitmap_node *marked_page = get_bitmap(_marked_page_id, txn);
        marked_page->header().set_inserted(_marked_bit_idx);
        marked_page->header().set_hasnt_label(false);

        node_base* parent = get_parent(marked_page, txn);
        update_inserted(parent, marked_page->parent_idx(), true, false, _allocator, txn);
        _marked_page_id.clear();
    }

    csize_t page_offset = offset1;

    page->reindex(base_prefix.inserted, base_prefix.hasnt_label, base_prefix.count, page_offset, get_node_bits(), get_data_bits());

    if (page->header().get_inserted() >= 0)
    {
        base_prefix.inserted = true;
        base_prefix.hasnt_label = page->header().hasnt_label();
    }
}

void remove_small_block(node_base *node, index_t idx,
                        index_t pos, index_t length, CountData &base_prefix,
                        int content_type, txn_t &txn)
{

    bitmap_node *bitmap = get_bitmap(node, idx, txn);

    index_t rank0 = bitmap->header().get_rank();
    index_t bitsize0 = bitmap->header().getBitsize();

    remove_data(bitmap, pos, length,
                base_prefix, content_type, txn);

    index_t rank1 = bitmap->header().get_rank();
    index_t bitsize1 = bitmap->header().getBitsize();

    update_btree_nodes(node, idx, -length, rank1 - rank0, bitsize1 - bitsize0, _allocator, txn);
}

void remove_bitmap_block(iterator<> &from, iterator<> &to, int content_type, txn_t &txn)
{

    if (from.data()->id() == to.data()->id())
    {
        CountData base_prefix = from.get_base_prefix();
        remove_small_block(from.page(), from.data()->parent_idx(),
                           from.idx(), to.idx() - from.idx(),
                           base_prefix, content_type, txn);
    }
    else
    {
        CountData base_prefix(0, false, false);
        node_base *start;
        index_t    start_idx;

        node_base *stop;
        index_t    stop_idx;

        if (from.idx() > 0 && !to.isEof())
        {
            if (to.idx() > 0)
            {
                base_prefix = to.get_base_prefix();

                remove_small_block(to.page(), to.data()->parent_idx(),
                                   0, to.idx(),
                                   base_prefix, content_type, txn);

            }

            if (to.data()->get_capacity() >= from.idx())
            {
                base_prefix = from.get_base_prefix();

                remove_small_block(from.page(), from.data()->parent_idx(),
                                   from.idx(), from.data()->header().get_size() - from.idx(),
                                   base_prefix, content_type, txn);

                move_data_in_page(from.data(), to.data(), 0, from.idx(), base_prefix, txn);

                get_start_page(from, start, start_idx, txn);
            }
            else
            {
                start = get_parent(from.data(), txn);
                start_idx = from.data()->parent_idx();
            }

            stop = get_parent(to.data(), txn);
            stop_idx = to.data()->parent_idx();
        }
        else
        {
            if (!to.isEof())
            {
                if (to.idx() > 0)
                {
                    base_prefix = to.get_base_prefix();
                    remove_small_block(to.page(), to.data()->parent_idx(),
                                       0, to.idx(),
                                       base_prefix, content_type, txn);
                }

                stop = get_parent(to.data(), txn);
                stop_idx = to.data()->parent_idx();
            }
            else
            {
                stop = get_parent(to.data(), txn);
                stop_idx = to.data()->parent_idx() + 1;
            }

            if (from.idx() > 0)
            {
                start = get_parent(from.data(), txn);
                start_idx = from.data()->parent_idx();
            }
            else
            {
                get_start_page(from, start, start_idx, txn);
            }
        }

        remove_pages(start, start_idx, stop, stop_idx, txn);
    }
}
