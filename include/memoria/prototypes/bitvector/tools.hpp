
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




template <typename NodePage1, typename NodePage2>
node_base *node2node(NodePage1 *src, bool root)
{
    long buffer[Allocator::PAGE_SIZE/sizeof(long)];
    NodePage2 *tgt = T2T<NodePage2*>(&buffer);

    tgt->flags()        = src->flags();
    tgt->id()           = src->id();
    tgt->parent_id()    = src->parent_id();
    tgt->parent_idx()   = src->parent_idx();
    tgt->level()        = src->level();
    tgt->page_count()   = src->page_count();

    tgt->set_root(root);

    for (index_t c = 0; c < src->children_count(); c++)
    {
        for (index_t d = 0; d < src->INDEXES; d++)
        {
            tgt->map().key(d, c) = src->map().key(d, c);
        }
        tgt->map().data(c) = src->map().data(c);
    }
    tgt->children_count() = src->children_count();

    for (index_t c = tgt->children_count(); c < tgt->map().maxSize(); c++)
    {
        for (index_t d = 0; d < src->INDEXES; d++)
        {
            tgt->map().key(d, c) = 0;
        }
        tgt->map().data(c) = 0;
    }

    tgt->map().reindex();

    long* psrc = (long*)src;
    for (index_t c = 0; c < Allocator::PAGE_SIZE/(long)sizeof(long); c++)
    {
        psrc[c] = buffer[c];
    }

    return (NodePage2*)psrc;
}

node_base *root2node(node_base *node)
{
    BV_NODE_CAST2(node, return node2node<root_node_t BV_COMMA node_t>((root_node_t*)__node, false));
}

node_base *node2root(node_base *node)
{
    BV_NODE_CAST2(node, return node2node<node_t BV_COMMA root_node_t>((node_t*)__node, true));
}

void copy_root_metadata(node_base *src, node_base *tgt)
{
    BV_ROOT_CAST2(src,
                  BV_ROOT_CAST2(tgt, __tgt->root_metadata() = __src->root_metadata())
                 );
}

bool can_convert_to_root(node_base *node)
{
    index_t node_max_size;

    if (node->get_key_size() == KEY_SIZE__SMALL)
    {
        node_max_size = root_small_node::map_t::maxSize();
    }
    else if (node->get_key_size() == KEY_SIZE__MEDIUM)
    {
        node_max_size = root_medium_node::map_t::maxSize();
    }
    else if (node->get_key_size() == KEY_SIZE__LARGE)
    {
        node_max_size = root_large_node::map_t::maxSize();
    }
    else
    {
        node_max_size = root_huge_node::map_t::maxSize();
    }

    //TODO Why ROOT_CAST ?
    BV_ROOT_CAST2(node, return __node->children_count() <= node_max_size);
}


static void dump(node_base *node, index_t i = 0)
{
    BV_NODE_CAST2(node, __node->map().dump(i, std::cout));
    std::cout<<node<<" "<<(void*)node->parent_id().value()<<":"<<node->parent_idx()<<" root: "<<node->is_root()<<" leaf: "<<node->is_leaf()<<" level: "<<node->level()<<std::endl;
}

static void dump_tree(node_base *node, Allocator &allocator, txn_t &txn, int idx = 0)
{
    dump(node, idx);
    for (index_t c = 0; c < get_children_count(node); c++)
    {
        if (!node->is_leaf())
        {
            node_base *child = get_child(node, c, allocator, txn);
            if (child!=NULL)
            {
                dumpTree(child, allocator, txn, idx);
            }
        }
    }
}

void dump_tree(txn_t &txn)
{
    dumpTree(get_node(_root, txn), _allocator, txn);
}

static node_base* get_child(node_base *node, index_t idx, Allocator &allocator, txn_t &txn)
{
    BV_NODE_CAST2(node, return static_cast<node_base*>(allocator.get(txn, __node->map().data(idx))));
}

static ID& get_page_id(node_base *node, index_t idx)
{
    BV_NODE_CAST2(node, return __node->map().data(idx));
}

bitmap_node* get_bitmap(node_base *node, index_t idx, txn_t &txn)
{
    BV_NODE_CAST2(node,
        return static_cast<bitmap_node*>(_allocator.get(txn, __node->map().data(idx)))
    );
}

static bitmap_node* get_bitmap(node_base *node, index_t idx, Allocator &allocator, txn_t &txn)
{
    BV_NODE_CAST2(node,
        return static_cast<bitmap_node*>(allocator.get(txn, __node->map().data(idx)))
    );
}

static node_base* get_last_child(node_base *node, Allocator &allocator, txn_t &txn)
{
    BV_NODE_CAST2(node,
        return static_cast<node_base*>(allocator.get(txn, __node->map().data(__node->children_count() - 1)))
    );
}

node_base* get_parent(bitmap_node *bitmap, txn_t &txn)
{
    return static_cast<node_base*>(_allocator.get(txn, bitmap->parent_id()));
}

static node_base* get_parent(bitmap_node *bitmap, Allocator &allocator, txn_t &txn)
{
    return static_cast<node_base*>(allocator.get(txn, bitmap->parent_id()));
}

node_base* get_parent(node_base *node, txn_t &txn)
{
    if (node->is_root())
    {
        return NULL;
    }
    else
    {
        return static_cast<node_base*>(_allocator.get(txn, node->parent_id()));
    }
}

index_t get_capacity(node_base *node)
{
    BV_NODE_CAST2(node,
        if (_max_node_capacity == -1)
        {
            return (__node->map().maxSize() - __node->children_count());
        }
        else
        {
            index_t capacity = _max_node_capacity - __node->children_count();
            return capacity > 0 ? capacity : 0;
        }
    );
}

index_t get_max_capacity(node_base *node)
{
    BV_NODE_CAST2(node,
        if (_max_node_capacity == -1)
        {
            return __node->map().maxSize();
        }
        else
        {
            return _max_node_capacity;
        }
    );
}

node_base* get_node(ID &id, txn_t &txn)
{
    return static_cast<node_base*>(_allocator.get(txn, id));
}

bitmap_node* get_bitmap(ID &id, txn_t &txn)
{
    return static_cast<bitmap_node*>(_allocator.get(txn, id));
}

static node_base* get_parent(node_base *node, Allocator &allocator, txn_t &txn)
{
    return static_cast<node_base*>(allocator.get(txn, node->parent_id()));
}

static index_t get_children_count(node_base *page)
{
    BV_NODE_CAST2(page, return __page->children_count());
}

static csize_t get_key(node_base *page, index_t i, index_t idx)
{
    BV_NODE_CAST2(page, return __page->map().key(i, idx));
}

static csize_t get_max_key(node_base *page, index_t i)
{
    BV_NODE_CAST2(page, return __page->map().max_key(i));
}

void reindex(node_base *page)
{
    BV_NODE_CAST2(page, return __page->map().reindex());
}

node_base* get_next_leaf(node_base *node, txn_t &txn)
{
    iterator<> iter(_allocator, txn, get_bitmap(node, 0, txn), 0);
    if (iter.next_leaf())
    {
        return iter.page();
    }
    else
    {
        return NULL;
    }
}

node_base* get_prev_leaf(node_base *node, txn_t &txn)
{
    iterator<> iter(_allocator, txn, node, 0);
    if (iter.prev_leaf())
    {
        return iter.page();
    }
    else
    {
        return NULL;
    }
}

node_base* get_root(txn_t &txn)
{
    return static_cast<node_base *>(_allocator.get(txn, _root));
}
