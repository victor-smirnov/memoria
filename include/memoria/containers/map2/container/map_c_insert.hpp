
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_C_INSERT_HPP
#define _MEMORIA_MODELS_IDX_MAP2_C_INSERT_HPP


#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrInsertName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Base::Accumulator                                          Accumulator;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;


    void insertEntry(Iterator& iter, const Element&);

    bool insert(Iterator& iter, const Element& element)
    {
    	insertEntry(iter, element);
    	return iter.nextKey();
    }

    void splitLeafPath(TreePath& left, TreePath& right, Int idx);
    TreePathItem splitLeaf(TreePath& path, Int idx);
    void         splitLeaf(TreePath& left, TreePath& right, Int idx);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map2::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
typename M_TYPE::TreePathItem M_TYPE::splitLeaf(TreePath& path, Int idx)
{
    Int level = 0;

	NodeBaseG& node     = path[level].node();
    NodeBaseG& parent   = path[level + 1].node();

    Int parent_idx      = path[level].parent_idx();

    node.update();
    parent.update();

    NodeBaseG other = me()->createNode(level, false, true, node->page_size());

    Accumulator keys = me()->moveElements(node, other, idx);

    //FIXME:: Make room in the parent
    me()->makeRoom(path, level + 1, parent_idx + 1, 1);

    me()->setINodeData(parent, parent_idx + 1, &other->id());

    //FIXME: Should we proceed up to the root here in general case?
    me()->updateCounters(parent, parent_idx,    -keys);
    me()->updateCounters(parent, parent_idx + 1, keys, true);

    return TreePathItem(other, parent_idx + 1);
}


M_PARAMS
void M_TYPE::splitLeaf(TreePath& left, TreePath& right, Int idx)
{
    Int level = 0;

	NodeBaseG& left_node    = left[level].node();

    NodeBaseG& left_parent  = left[level + 1].node();
    NodeBaseG& right_parent = right[level + 1].node();

    left_node.update();
    left_parent.update();
    right_parent.update();

    NodeBaseG other = me()->createNode(level, false, true, left_node->page_size());

    Accumulator keys = me()->moveElements(left_node, other, idx);

    Int parent_idx = left[level].parent_idx();

    if (right_parent == left_parent)
    {
        me()->makeRoom(left, level + 1,  parent_idx + 1, 1);
        me()->setChildID(left_parent, parent_idx + 1, other->id());

        //FIXME: should we proceed up to the root?
        me()->updateCounters(left_parent, parent_idx,    -keys);
        me()->updateCounters(left_parent, parent_idx + 1, keys, true);

        right[level].node()         = other;
        right[level].parent_idx()   = parent_idx + 1;
    }
    else {
    	me()->makeRoom(right, level + 1, 0, 1);
        me()->setChildID(right_parent, 0, other->id());

        me()->updateUp(left,  level + 1, parent_idx, -keys);
        me()->updateUp(right, level + 1, 0,           keys, true);

        right[level].node()         = other;
        right[level].parent_idx()   = 0;
    }
}



M_PARAMS
void M_TYPE::splitLeafPath(TreePath& left, TreePath& right, Int idx)
{
    Int level = 0;

	if (level < left.getSize() - 1)
    {
        NodeBaseG& parent = left[level + 1].node();

        if (me()->getCapacity(parent) == 0)
        {
            Int idx_in_parent = left[level].parent_idx();
            me()->splitPath(left, right, level + 1, idx_in_parent + 1);
        }

        splitLeaf(left, right, idx);
    }
    else
    {
        me()->newRoot(left);

        right.resize(left.getSize());
        right[level + 1] = left[level + 1];

        splitLeaf(left, right, idx);
    }
}



M_PARAMS
void M_TYPE::insertEntry(Iterator &iter, const Element& element)
{
    TreePath&   path    = iter.path();
    NodeBaseG&  node    = path.leaf().node();
    Int&        idx     = iter.key_idx();

    if (me()->getCapacity(node) > 0)
    {
        me()->makeRoom(path, 0, idx, 1);
    }
    else if (idx == 0)
    {
        TreePath next = path;
        me()->splitLeafPath(path, next, node->children_count() / 2);
        idx = 0;

        me()->makeLeafRoom(path, idx, 1);
    }
    else if (idx < node->children_count())
    {
        //FIXME: does it necessary to split the page at the middle ???
        TreePath next = path;
        me()->splitLeafPath(path, next, idx);
        me()->makeLeafRoom(path, idx, 1);
    }
    else {
        TreePath next = path;

        me()->splitLeafPath(path, next, node->children_count() / 2);

        path = next;

        idx = node->children_count();
        me()->makeLeafRoom(path, idx, 1);

        iter.key_idx()  = idx;
    }

    me()->setLeafDataAndReindex(node, idx, element);

    me()->updateParentIfExists(path, 0, element.first);

    me()->addTotalKeyCount(1);
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
