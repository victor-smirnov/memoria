
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_WALK_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_WALK_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>
#include <memoria/prototypes/btree/pages/pages.hpp>
#include <memoria/metadata/tools.hpp>


namespace memoria    {

using namespace memoria::btree;


MEMORIA_CONTAINER_PART_BEGIN(memoria::btree::WalkName);

    typedef typename Base::NodeBase                                                 NodeBase;
    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::NodeDispatcher                                           NodeDispatcher;
    typedef typename Base::Allocator                                                Allocator;

    typedef typename Base::TreePath                                                 TreePath;
    typedef typename Base::TreePathItem                                             TreePathItem;

    template <typename Walker>
    class WalkHelperFn
    {
        Int     result_;
        Walker& walker_;
        Int     idx_;
    public:
        WalkHelperFn(Walker& walker, Int idx): walker_(walker), idx_(idx) {}

        template <typename Node>
        void operator()(Node* node)
        {
            result_ = walker_(node, idx_);
        }

        Int result() const {
            return result_;
        }
    };

    /**
     * returns true if EOF, else false
     *
     */

    template <typename Walker>
    bool walkFw(TreePath& path, Int &idx, Walker &walker)
    {
        NodeBaseG index = path[0].node();

        // Walk up
        while (true)
        {
            WalkHelperFn<Walker> fn(walker, idx);
            NodeDispatcher::Dispatch(index, fn);

            if (fn.result() == -1)
            {
                if (!index->is_root())
                {
                    // The case when index->parent_idx() == parent.size
                    // should be handled correctly in the walker
                    idx     = path[index->level()].parent_idx() + 1;
                    index   = me()->getParent(path, index);
                }
                else {
                    // EOF
                    idx     = -1;
                    break;
                }
            }
            else {
                idx = fn.result();
                break;
            }
        }

        if (idx != -1)
        {
            // walk down
            while (true)
            {
                WalkHelperFn<Walker> fn(walker, idx);
                NodeDispatcher::Dispatch(index, fn);

                if (fn.result() == -1)
                {
                    // this should not happened here
                    throw Exception(MEMORIA_SOURCE, "walkFw:Down: idx == -1");
                }

                if (!index->is_leaf())
                {
                    index = me()->getChild(index, fn.result(), Allocator::READ);

                    path[index->level()].node()         = index;
                    path[index->level()].parent_idx()   = fn.result();

                    idx = 0; // FIXME: check this
                }
                else {
                    idx = fn.result();
                    return false;
                }
            }
        }
        else {
            // END
            while (true)
            {
                if (!index->is_leaf())
                {
                    Int parent_idx = index->children_count() - 1;
                    index = me()->getLastChild(index, Allocator::READ);

                    path[index->level()].node()         = index;
                    path[index->level()].parent_idx()   = parent_idx;
                }
                else {
                    //FIXME: remove '-1'
                    idx = index->children_count() - 1;
                    return true;
                }
            }
        }
    }

    /**
     * returns true if BOF else false
     *
     */

    template <typename Walker>
    bool walkBw(TreePath& path, Int &idx, Walker &walker)
    {
        NodeBaseG index = path[0].node();

        // Walk up
        while (true)
        {
            WalkHelperFn<Walker> fn(walker, idx);
            NodeDispatcher::Dispatch(index, fn);

            if (fn.result() == -1)
            {
                if (!index->is_root())
                {

                    idx     = path[index->level()].parent_idx() - 1;
                    index   = me()->getParent(path, index);
                }
                else {
                    // START
                    idx     = -1;
                    break;
                }
            }
            else {
                idx = fn.result();
                break;
            }
        }

        if (idx != -1)
        {
            // walk down
            while (true)
            {
                WalkHelperFn<Walker> fn(walker, idx);
                NodeDispatcher::Dispatch(index, fn);

                if (!index->is_leaf())
                {
                    index   = me()->getChild(index, fn.result(), Allocator::READ);

                    path[index->level()].node()         = index;
                    path[index->level()].parent_idx()   = fn.result();

                    idx     = index->children_count() - 1;
                }
                else {
                    idx = fn.result();
                    return false;
                }
            }
        }
        else {
            // START
            idx = 0;
            while (true)
            {
                if (!index->is_leaf())
                {
                    index = me()->getChild(index, 0, Allocator::READ);

                    path[index->level()].node()         = index;
                    path[index->level()].parent_idx()   = 0;
                }
                else {
                    return true;
                }
            }
        }
    }
    
    template <typename Walker>
    void walk_to_the_root(TreePath& path, Int idx, Walker& walker, Int level = 0)
    {
        for (Int c = level; c < path.getSize(); c++)
        {
            walker(path[c].node(), idx);

            idx = path[c].parent_idx();
        }
    }



MEMORIA_CONTAINER_PART_END

}

#endif
