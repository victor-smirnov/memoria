
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_REMOVE_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::RemoveName)

	typedef TypesType															Types;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::ID                                                   ID;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                         	Position;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int  Indexes                                                   = Base::Indexes;

    


	bool removeEntry(Iterator& iter, Accumulator& keys);

	void removeEntry(TreePath& path, Int stream, Int& idx, Accumulator& keys, bool merge = true);




MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::RemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



/**
 * \brief Remove the key and data pointed by iterator *iter* form the tree.
 *
 * This call stores removed key values in *keys* variable.
 *
 * \param iter iterator pointing to the key/data pair
 * \param keys an accumulator to add removed key value to
 *
 * \return true if the entry has been removed
 */

M_PARAMS
bool M_TYPE::removeEntry(Iterator& iter, Accumulator& keys)
{
	auto& self = this->self();

	if (iter.isNotEmpty() || iter.isNotEnd())
    {
        self.removeEntry(iter.path(), iter.stream(), iter.idx(), keys);

        if (iter.isEnd())
        {
            iter.nextLeaf();
        }

        return true;
    }
    else {
        return false;
    }
}

/**
 * \brief Remove single entry from the leaf node.
 *
 * \param path  path to the leaf
 * \param idx   index on the entry in the leaf
 * \param keys  accumulator to store values of deleted entry
 * \param merge if true then merge leaf with its siblings (if necessary)
 *
 * \see mergeWithSiblings
 */

M_PARAMS
void M_TYPE::removeEntry(TreePath& path, Int stream, Int& idx, Accumulator& keys, bool merge)
{
    auto& self = this->self();

	Int children_count  = self.getNodeSize(path.leaf(), stream);

    //if leaf page has more than 1 key do regular remove

    if (children_count > 1)
    {
        //remove 1 element from the leaf, update parent and
        //do not try to remove children (it's a leaf)

        self.removeRoom(path, 0, Position::create(stream, idx), Position::create(stream, 1), keys);

        //try merging this leaf with previous of following
        //leaf if filled by half of it's capacity.
        if (merge && self.shouldMergeNode(path, 0))
        {
        	Position idxp(idx);
            self.mergeWithSiblings(path, 0, idxp);
        	idx = idxp[stream];
        }
    }
    else {
        keys = self.getLeafKeys(path.leaf().node(), idx);
        self.removePage1(path, stream, idx);
    }

    self.addTotalKeyCount(Position::create(stream, -1));
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
