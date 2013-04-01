
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_C_REMOVE_HPP
#define _MEMORIA_MODELS_IDX_MAP2_C_REMOVE_HPP


#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrRemoveName)

	typedef typename Base::WrappedCtr::Types                                  	WTypes;
	typedef typename Base::WrappedCtr::Allocator                              	Allocator;

	typedef typename Base::WrappedCtr::ID                                     	ID;

	typedef typename WTypes::NodeBase                                           NodeBase;
	typedef typename WTypes::NodeBaseG                                          NodeBaseG;

	typedef typename Base::Iterator                                             Iterator;

	typedef typename WTypes::Pages::NodeDispatcher                              NodeDispatcher;
	typedef typename WTypes::Pages::RootDispatcher                              RootDispatcher;
	typedef typename WTypes::Pages::LeafDispatcher                              LeafDispatcher;
	typedef typename WTypes::Pages::NonLeafDispatcher                           NonLeafDispatcher;


	typedef typename WTypes::Key                                                Key;
	typedef typename WTypes::Value                                              Value;
	typedef typename WTypes::Element                                            Element;

	typedef typename WTypes::Metadata                                           Metadata;

	typedef typename WTypes::Accumulator                                        Accumulator;

	typedef typename WTypes::TreePath                                           TreePath;
	typedef typename WTypes::TreePathItem                                       TreePathItem;


    bool removeEntry(Iterator& iter, Accumulator& keys);

    void removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge = true);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map2::CtrRemoveName)
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
    if (iter.isNotEmpty() || iter.isNotEnd())
    {
        removeEntry(iter.iter().path(), iter.iter().key_idx(), keys);

        if (iter.isEnd())
        {
            iter.iter().nextLeaf();
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
void M_TYPE::removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge)
{
    auto& ctr = self().ctr();

	Int children_count  = path.leaf()->children_count();

    //if leaf page has more than 1 key do regular remove

    if (children_count > 1)
    {
        //remove 1 element from the leaf, update parent and
        //do not try to remove children (it's a leaf)

        ctr.removeRoom(path, 0, idx, 1, keys);

        //try merging this leaf with previous of following
        //leaf if filled by half of it's capacity.
        if (merge && ctr.shouldMergeNode(path, 0))
        {
            ctr.mergeWithSiblings(path, 0, idx);
        }

        ctr.finishPathStep(path, idx);
    }
    else {
        keys = self().getLeafKeys(path.leaf().node(), idx);
        ctr.removePage(path, idx);
    }

    ctr.addTotalKeyCount(-1);
}

}

#undef M_TYPE
#undef M_PARAMS


#endif
