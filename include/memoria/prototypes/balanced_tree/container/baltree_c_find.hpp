
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP

#include <memoria/prototypes/balanced_tree/baltree_types.hpp>

#include <memoria/core/container/macros.hpp>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::FindName)

    typedef TypesType                                                           Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Position                                            Position;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;
    typedef typename Base::LeafDispatcher                                    	LeafDispatcher;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::TreePath                                             TreePath;

    static const Int MAIN_STREAM												= Types::MAIN_STREAM;


    struct SearchModeDefault {
        typedef enum {NONE, FIRST, LAST} Enum;
    };

private:


public:

    template <typename Walker>
    Iterator find0(Int stream, Walker&& walker);

    template <typename Walker>
    void find1(Walker&& walker);

    template <typename Walker>
    Int findFw(TreePath& path, Int stream, Int idx, Walker&& walker, Int level = 0);

    template <typename Walker>
    Int findBw(TreePath& path, Int stream, Int idx, Walker&& walker, Int level = 0);

    MEMORIA_PUBLIC Position sizes() const
    {
    	return self().getTotalKeyCount();
    }

    MEMORIA_PUBLIC Iterator streamBegin(Int stream)
    {
    	typename Types::template FindBeginWalker<Types> walker(stream, self());
    	return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator Begin()
    {
    	return streamBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator begin()
    {
    	return streamBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator streamRBegin(Int stream)
    {
    	typename Types::template FindRBeginWalker<Types> walker(stream, self());
    	return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator RBegin()
    {
    	return streamRBegin(MAIN_STREAM);
    }

    MEMORIA_PUBLIC Iterator streamEnd(Int stream)
    {
    	typename Types::template FindEndWalker<Types> walker(stream, self());
    	return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator End()
    {
    	return streamEnd(MAIN_STREAM);
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

    MEMORIA_PUBLIC Iterator streamREnd(Int stream)
    {
    	typename Types::template FindREndWalker<Types> walker(stream, self());
    	return self().find0(stream, walker);
    }

    MEMORIA_PUBLIC Iterator REnd()
    {
    	return streamREnd(MAIN_STREAM);
    }

    Iterator findLT(Int stream, Key key, Int key_num)
    {
    	typename Types::template FindLTWalker<Types> walker(stream, key_num, key);

    	return self().find0(stream, walker);
    }

    Iterator findLE(Int stream, Key key, Int key_num)
    {
    	typename Types::template FindLEWalker<Types> walker(stream, key_num, key);

    	return self().find0(stream, walker);
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::FindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
template <typename Walker>
typename M_TYPE::Iterator M_TYPE::find0(Int stream, Walker&& walker)
{
	auto& self = this->self();

	walker.direction() 	= WalkDirection::DOWN;

	NodeBaseG node = self.getRoot(Allocator::READ);
	if (node.isSet())
	{
		Iterator i(self, node->level() + 1);

		i.stream() = stream;
		i.setNode(node, 0);

		Int size = self.getNodeSize(node, stream);

		if (size > 0)
		{
			bool out_of_range = false;

			while (!node->is_leaf())
			{
				Int idx;
				if (!out_of_range)
				{
					idx = NodeDispatcher::dispatchConstRtn(node, walker, 0);

					size = self.getNodeSize(node, stream);

					if (idx >= size)
					{
						out_of_range = true;
						idx = size - 1;
					}
				}
				else {
					idx = self.getNodeSize(node, stream) - 1;
				}

				node = me()->getChild(node, idx, Allocator::READ);
				i.setNode(node, idx);
			}

			Int idx;
			if (!out_of_range)
			{
				i.key_idx() = idx = NodeDispatcher::dispatchConstRtn(node, walker, 0);
			}
			else {
				i.key_idx() = idx = self.getNodeSize(node, stream);
			}

			walker.finish(i, idx);
		}
		else {
			walker.empty(i);
		}

		return i;
	}
	else {
		return Iterator(self);
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
Int M_TYPE::findFw(TreePath& path, Int stream, Int start, Walker&& walker, Int level)
{
	auto& self = this->self();

	NodeBaseG node = path[level].node();

	if (node->is_root())
	{
		walker.direction() 	= WalkDirection::DOWN;
	}
	else {
		walker.direction() 	= WalkDirection::UP;
	}

	Int size = self.getNodeSize(node, stream);

	Int idx;

	if (start < size)
	{
		idx = NodeDispatcher::dispatchConstRtn(node, walker, start);
	}
	else {
		idx = size;
	}

	if (idx >= size)
	{
		if (!node->is_root())
		{
			// Step up the tree
			Int child_idx = findFw(path, stream, path[level].parent_idx() + 1, walker, level + 1);


			Int parent_size = self.getNodeSize(path[level + 1].node(), stream);
			if (child_idx < parent_size)
			{
				// Step down the tree
				NodeBaseG child_node		= self.getChild(path[level + 1].node(), child_idx, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= child_idx;

				return NodeDispatcher::dispatchConstRtn(child_node, walker, 0);
			}
			else {
				// Step down the tree
				NodeBaseG child_node		= self.getChild(path[level + 1].node(), parent_size - 1, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= parent_size - 1;

				return self.getNodeSize(child_node, stream);
			}
		}
		else {
			walker.direction() 	= WalkDirection::DOWN;
			return size;
		}
	}
	else {
		walker.direction() 	= WalkDirection::DOWN;
		return idx;
	}
}





M_PARAMS
template <typename Walker>
Int M_TYPE::findBw(TreePath& path, Int stream, Int start, Walker&& walker, Int level)
{
	auto& self = this->self();

	NodeBaseG node = path[level].node();

	if (node->is_root())
	{
		walker.direction() 	= WalkDirection::DOWN;
	}
	else {
		walker.direction() 	= WalkDirection::UP;
	}

	Int idx;

	if (start >= 0)
	{
		idx = NodeDispatcher::dispatchConstRtn(node, walker, start);
	}
	else {
		idx = -1;
	}

	if (idx < 0)
	{
		if (!node->is_root())
		{
			// Step up the tree
			Int child_idx = findBw(path, stream, path[level].parent_idx() - 1, walker, level + 1);

			if (child_idx >= 0)
			{
				// Step down the tree
				NodeBaseG child_node		= self.getChild(path[level + 1].node(), child_idx, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= child_idx;

				Int start = self.getNodeSize(child_node, stream) - !child_node->is_leaf();

				return NodeDispatcher::dispatchConstRtn(child_node, walker, start);
			}
			else {
				// Step down the tree
				NodeBaseG child_node		= self.getChild(path[level + 1].node(), 0, Allocator::READ);

				path[level].node() 			= child_node;
				path[level].parent_idx()	= 0;

				return -1;
			}
		}
		else {
			return -1;
		}
	}
	else {
		walker.direction() 	= WalkDirection::DOWN;
		return idx;
	}
}



#undef M_TYPE
#undef M_PARAMS

}

#endif  //_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
