
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

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;


	bool removeEntry(Iterator& iter, Accumulator& keys);

	void removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge = true);

	bool removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map2::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys)
{
	auto& ctr = self();

	auto& from_path 	= from.path();
	Position from_pos 	= Position(from.entry_idx());

	auto& to_path 		= to.path();
	Position to_pos 	= Position(to.entry_idx());

	bool result = ctr.removeEntries(from_path, from_pos, to_path, to_pos, keys, true);

	from.key_idx() = to.key_idx() = to_pos.get();

	return result;
}


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
        removeEntry(iter.path(), iter.key_idx(), keys);

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
void M_TYPE::removeEntry(TreePath& path, Int& idx, Accumulator& keys, bool merge)
{
    auto& ctr = self();

	Int children_count  = ctr.getNodeSize(path.leaf(), 0);

    //if leaf page has more than 1 key do regular remove

    if (children_count > 1)
    {
        //remove 1 element from the leaf, update parent and
        //do not try to remove children (it's a leaf)

        ctr.removeRoom(path, 0, Position(idx), Position(1), keys);

        //try merging this leaf with previous of following
        //leaf if filled by half of it's capacity.
        if (merge && ctr.shouldMergeNode(path, 0))
        {
        	Position idxp(idx);
            ctr.mergeWithSiblings(path, 0, idxp);
        	idx = idxp.get();
        }
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
