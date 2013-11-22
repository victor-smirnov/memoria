
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_FIND_HPP

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/container/macros.hpp>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::FindName)

    typedef TypesType                                                           Types;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Position                                            Position;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::Key                                                  Key;
    typedef typename Base::TreePath                                             TreePath;

    static const Int MAIN_STREAM                                                = Types::MAIN_STREAM;


private:


public:

    template <typename Walker>
    Iterator find0(Int stream, Walker&& walker);

    template <typename Walker>
    Int findFw(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

    template <typename Walker>
    Int findBw(NodeBaseG& node, Int stream, Int idx, Walker&& walker);

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

    Iterator findGT(Int stream, Key key, Int key_num)
    {
        typename Types::template FindGTWalker<Types> walker(stream, key_num, key);

        return self().find0(stream, walker);
    }

    Iterator findGE(Int stream, Key key, Int key_num)
    {
        typename Types::template FindGEWalker<Types> walker(stream, key_num, key);

        return self().find0(stream, walker);
    }

    template <typename Walker>
    void walkUp(NodeBaseG node, Int idx, Walker&& walker) const
    {
        NodeDispatcher::dispatchConst(node, walker, idx);

        while (!node->is_root())
        {
            idx = node->parent_idx();
            node = self().getNodeParent(node);

            NodeDispatcher::dispatchConst(node, walker, idx);
        }
    }

MEMORIA_CONTAINER_PART_END



#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::FindName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
template <typename Walker>
typename M_TYPE::Iterator M_TYPE::find0(Int stream, Walker&& walker)
{
	auto& self = this->self();
    walker.direction()  = WalkDirection::DOWN;

    NodeBaseG node = self.getRoot();
    if (node.isSet())
    {
        Iterator i(self);

        i.stream() = stream;

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

                node = me()->getChild(node, idx);
            }

            Int idx;
            if (!out_of_range)
            {
                i.idx() = idx = NodeDispatcher::dispatchConstRtn(node, walker, 0);
            }
            else {
                i.idx() = idx = self.getNodeSize(node, stream);
            }

            i.leaf() = node;

            walker.finish(i, idx);
        }
        else {
            i.leaf() = node;

            walker.empty(i);
        }

        i.init();
        return i;
    }
    else {
        return Iterator(self);
    }
}



M_PARAMS
template <typename Walker>
Int M_TYPE::findFw(NodeBaseG& node, Int stream, Int start, Walker&& walker)
{
    auto& self = this->self();

    if (node->is_root())
    {
        walker.direction()  = WalkDirection::DOWN;
    }
    else {
        walker.direction()  = WalkDirection::UP;
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
            NodeBaseG parent = self.getNodeParent(node);

            // Step up the tree
            Int child_idx = findFw(parent, stream, node->parent_idx() + 1, walker);

            Int parent_size = self.getNodeSize(parent, stream);
            if (child_idx < parent_size)
            {
                // Step down the tree
                node = self.getChild(parent, child_idx);

                return NodeDispatcher::dispatchConstRtn(node, walker, 0);
            }
            else {
                // Step down the tree
                node = self.getChild(parent, parent_size - 1);

                return self.getNodeSize(node, stream);
            }
        }
        else {
            walker.direction()  = WalkDirection::DOWN;
            return size;
        }
    }
    else {
        walker.direction()  = WalkDirection::DOWN;
        return idx;
    }
}





M_PARAMS
template <typename Walker>
Int M_TYPE::findBw(NodeBaseG& node, Int stream, Int start, Walker&& walker)
{
    auto& self = this->self();

    if (node->is_root())
    {
        walker.direction()  = WalkDirection::DOWN;
    }
    else {
        walker.direction()  = WalkDirection::UP;
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
            NodeBaseG parent = self.getNodeParent(node);

            // Step up the tree
            Int child_idx = findBw(parent, stream, node->parent_idx() - 1, walker);

            if (child_idx >= 0)
            {
                // Step down the tree
                node        = self.getChild(parent, child_idx);
                Int start   = self.getNodeSize(node, stream) - !node->is_leaf();

                return NodeDispatcher::dispatchConstRtn(node, walker, start);
            }
            else {
                // Step down the tree
                node = self.getChild(parent, 0);

                return -1;
            }
        }
        else {
            return -1;
        }
    }
    else {
        walker.direction()  = WalkDirection::DOWN;
        return idx;
    }
}



#undef M_TYPE
#undef M_PARAMS

}

#endif  //_MEMORIA_MODELS_KVMAP_MODEL_FIND_HPP
