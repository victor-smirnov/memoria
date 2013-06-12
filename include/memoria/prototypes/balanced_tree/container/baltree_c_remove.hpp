
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

	void removeEntry(NodeBaseG& node, Int stream, Int& idx, Accumulator& keys, bool merge = true);

	void removeEntryP(NodeBaseG& node, Int stream, Int& idx, Accumulator& keys, bool merge = true)
	{}



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
        self.removeEntry(iter.leaf(), iter.stream(), iter.idx(), keys);

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
void M_TYPE::removeEntry(NodeBaseG& node, Int stream, Int& idx, Accumulator& keys, bool merge)
{
    auto& self = this->self();

    MEMORIA_ASSERT_TRUE(node->is_leaf());

    VectorAdd(keys, self.removeLeafContent(node, stream, idx, idx + 1));

    if (merge && self.shouldMergeNode(node))
    {
    	self.mergeWithSiblings(node, [&, stream](const NodeBaseG& left, const NodeBaseG& right) {
    		if (left->is_leaf())
    		{
    			idx += self.getNodeSize(left, stream);
    		}
    	});
    }

    Position sizes = self.getNodeSizes(node);

    if (sizes.eqAll(0))
    {
    	// TODO: find the nearest leaf for the specified stream
    	// remove empty leaf node
    }

    self.addTotalKeyCount(Position::create(stream, -1));
}


#undef M_TYPE
#undef M_PARAMS


}

#endif
