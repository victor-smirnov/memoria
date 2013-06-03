
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODENORM_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODENORM_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::balanced_tree;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::NodeNormName)

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

    typedef typename Types::PageUpdateMgr										PageUpdateMgr;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    void updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully = false);

    void updateParentIfExists(TreePath& path, Int level, const Accumulator& counters);

    void splitPath(TreePath& left, TreePath& right, Int level, const Position& idx, UBigInt active_streams);

    MEMORIA_DECLARE_NODE_FN(InsertFn, insert);

    void insertNonLeaf(
    		TreePath& path,
    		Int level,
    		Int idx,
    		const Accumulator& keys,
    		const ID& id
    );

    void insertNonLeaf(
    		NodeBaseG& node,
    		Int idx,
    		const Accumulator& keys,
    		const ID& id
    );


MEMORIA_CONTAINER_PART_END


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::NodeNormName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
void M_TYPE::updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex)
{
    auto& self = this->self();

	for (Int c = level; c < path.getSize(); c++)
    {
		if (self.updateCounters(path, c, idx, counters, reindex))
        {
            break;
        }
        else {
            idx = path[c].parent_idx();
        }
    }
}


M_PARAMS
void M_TYPE::updateParentIfExists(TreePath& path, Int level, const Accumulator& counters)
{
	auto& self = this->self();

	if (level < path.getSize() - 1)
    {
        self.updateUp(path, level + 1, path[level].parent_idx(), counters, true);
    }
}


M_PARAMS
void M_TYPE::insertNonLeaf(
		TreePath& path,
		Int level,
		Int idx,
		const Accumulator& keys,
		const ID& id
)
{
	auto& self = this->self();
	NodeBaseG& node = path[level];

	node.update();

	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
}


M_PARAMS
void M_TYPE::insertNonLeaf(
		NodeBaseG& node,
		Int idx,
		const Accumulator& keys,
		const ID& id
)
{
	node.update();
	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
}


M_PARAMS
void M_TYPE::splitPath(TreePath& left, TreePath& right, Int level, const Position& idx, UBigInt active_streams)
{
	auto& self = this->self();

	if (level == left.getSize() - 1)
	{
		self.newRoot(left);
		right.resize(left.getSize());
		right[level + 1] = left[level + 1];
	}

	NodeBaseG& left_node    = left[level].node();
	NodeBaseG& left_parent  = left[level + 1].node();

	left_node.update();
	left_parent.update();

	NodeBaseG other  = self.createNode1(level, false, left_node->is_leaf(), left_node->page_size());

	Accumulator keys = self.moveElements(left_node, other, idx);

	Int parent_idx   = left[level].parent_idx();

	self.updateUp(left, level + 1, parent_idx, -keys, true);

	if (self.getNonLeafCapacity(left_parent, active_streams) > 0)
	{
		self.insertNonLeaf(left_parent, parent_idx + 1, keys, other->id());
		self.updateUp(left, level + 2, left[level + 1].parent_idx(), keys, true);

		right[level].node()         = other;
		right[level].parent_idx()   = parent_idx + 1;
	}
	else {
		splitPath(left, right, level + 1, Position(parent_idx + 1), active_streams);

		self.insertNonLeaf(right[level + 1], 0, keys, other->id());
		self.updateUp(right, level + 2, right[level + 1].parent_idx(), keys);

		right[level].node()         = other;
		right[level].parent_idx()   = 0;
	}
}

#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
