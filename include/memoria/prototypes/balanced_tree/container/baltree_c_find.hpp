
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP

#include <memoria/prototypes/balanced_tree/baltree_names.hpp>

#include <memoria/core/container/macros.hpp>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::FindName)

    typedef TypesType                                                           Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::TreePath                                             TreePath;


    struct SearchModeDefault {
        typedef enum {NONE, FIRST, LAST} Enum;
    };

private:


public:

    template <typename Walker>
    Iterator find0(Walker&& walker);

    template <typename Walker>
    void find1(Walker&& walker);

    template <typename Walker>
    Int findFw(TreePath& path, Int idx, Walker&& walker, Int level = 0);

    template <typename Walker>
    Int findBw(TreePath& path, Int idx, Walker&& walker, Int level = 0);

    MEMORIA_PUBLIC MEMORIA_DEPRECATED BigInt getSize() const
    {
        return me()->getTotalKeyCount();
    }

    MEMORIA_PUBLIC BigInt size() const
    {
    	return me()->getTotalKeyCount();
    }

    MEMORIA_PUBLIC Iterator Begin()
    {
    	typename Types::template FindBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator begin()
    {
    	typename Types::template FindBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator RBegin()
    {
    	typename Types::template FindRBeginWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    MEMORIA_PUBLIC Iterator End()
    {
    	typename Types::template FindEndWalker<Types> walker(*me());
    	return me()->find0(walker);
    }


    MEMORIA_PUBLIC Iterator end()
    {
        Iterator iter(*me());
        iter.type() = Iterator::END;
        return iter;
    }

    MEMORIA_PUBLIC IterEndMark endm()
    {
        return IterEndMark();
    }

    MEMORIA_PUBLIC Iterator REnd()
    {
    	typename Types::template FindREndWalker<Types> walker(*me());
    	return me()->find0(walker);
    }

    Iterator findLT(Key key, Int key_num)
    {
    	typename Types::template FindLTWalker<Types> walker(key, key_num);

    	return me()->find0(walker);
    }

    Iterator findLE(Key key, Int key_num)
    {
    	typename Types::template FindLEWalker<Types> walker(key, key_num);

    	return me()->find0(walker);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::FindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
template <typename Walker>
typename M_TYPE::Iterator M_TYPE::find0(Walker&& walker)
{
	walker.start() 		= 0;
	walker.direction() 	= WalkDirection::DOWN;

	NodeBaseG node = me()->getRoot(Allocator::READ);
	if (node.isSet())
	{
		Iterator i(*me(), node->level() + 1);

		i.setNode(node, 0);

		if (node->children_count() > 0)
		{
			while (!node->is_leaf())
			{
				NodeDispatcher::dispatchConst(node, walker);

				Int idx = walker.idx();

				node = me()->getChild(node, idx, Allocator::READ);
				i.setNode(node, idx);
			}

			NodeDispatcher::dispatchConst(node, walker);

			Int idx = walker.idx();
			i.key_idx() = idx;

			me()->finishPathStep(i.path(), idx);

			walker.finish(idx, i);
		}
		else {
			walker.empty(i);
		}

		return i;
	}
	else {
		return Iterator(*me());
	}
}



M_PARAMS
template <typename Walker>
void M_TYPE::find1(Walker&& walker)
{
	walker.start() 		= 0;
	walker.direction() 	= WalkDirection::DOWN;

	NodeBaseG node = me()->getRoot(Allocator::READ);
	if (node.isSet())
	{
		if (node->children_count() > 0)
		{
			while (!node->is_leaf())
			{
				NodeDispatcher::dispatchConst(node, walker);

				Int idx = walker.idx();

				node = me()->getChild(node, idx, Allocator::READ);
			}

			NodeDispatcher::dispatchConst(node, walker);

			Int idx = walker.idx();
			walker.finish(*me(), node, idx);
		}
		else {
			walker.empty();
		}
	}
	else {
		walker.empty();
	}
}


M_PARAMS
template <typename Walker>
Int M_TYPE::findFw(TreePath& path, Int idx, Walker&& walker, Int level)
{
	NodeBaseG node = path[level].node();

	if (node->is_root())
	{
		walker.direction() 	= WalkDirection::DOWN;
	}
	else {
		walker.direction() 	= WalkDirection::UP;
	}

	walker.idx() = walker.start() = idx;

	if (idx < node->children_count())
	{
		NodeDispatcher::dispatchConst(node, walker);
	}

	if (walker.idx() >= node->children_count())
	{
		if (!node->is_root())
		{
			// Step up the tree
			Int child_idx = findFw(path, path[level].parent_idx() + 1, walker, level + 1);

			if (child_idx < path[level + 1].node()->children_count())
			{
				// Step down the tree
				NodeBaseG child_node		= me()->getChild(path[level + 1].node(), child_idx, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= child_idx;

				NodeDispatcher::dispatchConst(child_node, walker);

				return walker.idx();
			}
			else {
				return path[level].node()->children_count();
			}
		}
		else {
			walker.direction() 	= WalkDirection::DOWN;
			walker.start() 		= 0;

			return walker.idx();
		}
	}
	else {
		walker.direction() 	= WalkDirection::DOWN;
		walker.start() 		= 0;

		return walker.idx();
	}
}


M_PARAMS
template <typename Walker>
Int M_TYPE::findBw(TreePath& path, Int idx, Walker&& walker, Int level)
{
	NodeBaseG node = path[level].node();

	if (node->is_root())
	{
		walker.direction() 	= WalkDirection::DOWN;
	}
	else {
		walker.direction() 	= WalkDirection::UP;
	}

	walker.idx() = walker.start() = idx;

	if (idx >= 0)
	{
		NodeDispatcher::dispatchConst(node, walker);
	}

	if (walker.idx() < 0)
	{
		if (!node->is_root())
		{
			// Step up the tree
			Int child_idx			= findBw(path, path[level].parent_idx() - 1, walker, level + 1);

			if (child_idx >= 0)
			{
				// Step down the tree
				NodeBaseG child_node		= me()->getChild(path[level + 1].node(), child_idx, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= child_idx;

				walker.start() 				= child_node->children_count() - 1;

				NodeDispatcher::dispatchConst(child_node, walker);

				return walker.idx();
			}
			else {
				return -1;
			}
		}
		else if (node->is_leaf())
		{
			return walker.idx();
		}
		else {
			return -1;
		}
	}
	else {
		walker.direction() 	= WalkDirection::DOWN;
		walker.start() 		= 0;

		return walker.idx();
	}
}



#undef M_TYPE
#undef M_PARAMS

}

#endif  //_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
