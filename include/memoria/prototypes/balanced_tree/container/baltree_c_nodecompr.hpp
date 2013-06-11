
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODECOMPR_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MODEL_NODECOMPR_HPP

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::balanced_tree;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::balanced_tree::NodeComprName)

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

    typedef std::function<Accumulator (NodeBaseG&, NodeBaseG&)> 				SplitFn;

    static const Int Indexes                                                    = Types::Indexes;
    static const Int Streams                                                    = Types::Streams;

    void insertNonLeafP(NodeBaseG& node, Int idx, const Accumulator& keys, const ID& id);

    NodeBaseG splitPathP(NodeBaseG& node, Int split_at);
    NodeBaseG splitLeafP(NodeBaseG& leaf, const Position& split_at);

    NodeBaseG splitP(NodeBaseG& node, SplitFn split_fn);

    MEMORIA_DECLARE_NODE_FN(UpdateNodeFn, updateUp);
    bool updateNode(NodeBaseG& node, Int idx, const Accumulator& keys);
    void updatePath(NodeBaseG& node, Int& idx, const Accumulator& keys);
    void updatePathNoBackup(NodeBaseG& node, Int idx, const Accumulator& keys);






    void updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, std::function<void (Int, Int)> fn);


    void updateUpNoBackup(TreePath& path, Int level, Int idx, const Accumulator& counters);

    void updateParentIfExists(TreePath& path, Int level, const Accumulator& counters);

    bool updateCounters(
    		TreePath& path,
    		Int level,
    		Int idx,
    		const Accumulator& counters,
    		std::function<void (Int, Int)> fn
    );



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


#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::balanced_tree::NodeComprName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
void M_TYPE::insertNonLeafP(
		NodeBaseG& node,
		Int idx,
		const Accumulator& keys,
		const ID& id
)
{
	auto& self = this->self();

	node.update();
	NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
	self.updateChildren(node, idx + 1);

	if (node->is_root())
	{
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);
		self.updatePath(parent, node->parent_idx(), keys);
	}
}




M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitP(NodeBaseG& left_node, SplitFn split_fn)
{
	auto& self = this->self();

	if (left_node->is_root())
	{
		self.newRootP(left_node);
	}
	else {
		left_node.update();
	}

	NodeBaseG left_parent  = self.getNodeParent(left_node, Allocator::UPDATE);

	NodeBaseG other  = self.createNode1(left_node->level(), false, left_node->is_leaf(), left_node->page_size());

	Accumulator keys = split_fn(left_node, other);

	Int parent_idx   = left_node->parent_idx();

	self.updatePathNoBackup(left_parent, parent_idx, -keys);

	PageUpdateMgr mgr(self);
	mgr.add(left_parent);

	try {
		self.insertNonLeafP(left_parent, parent_idx + 1, keys, other->id());
	}
	catch (PackedOOMException ex)
	{
		mgr.rollback();

		NodeBaseG right_parent = splitPathP(left_parent, parent_idx + 1);

		mgr.add(right_parent);

		try {
			self.insertNonLeafP(right_parent, 0, keys, other->id());
		}
		catch (PackedOOMException ex2)
		{
			mgr.rollback();

			Int right_parent_size = self.getNodeSize(right_parent, 0);

			splitPathP(right_parent, right_parent_size / 2);

			self.insertNonLeafP(right_parent, 0, keys, other->id());
		}
	}

	return other;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitPathP(NodeBaseG& left_node, Int split_at)
{
	auto& self = this->self();

	return splitP(left_node, [&self, split_at](NodeBaseG& left, NodeBaseG& right){
		return self.splitNonLeafNode(left, right, split_at);
	});
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::splitLeafP(NodeBaseG& left_node, const Position& split_at)
{
	auto& self = this->self();

	return splitP(left_node, [&self, &split_at](NodeBaseG& left, NodeBaseG& right){
		return self.splitLeafNode(left, right, split_at);
	});
}

M_PARAMS
bool M_TYPE::updateNode(NodeBaseG& node, Int idx, const Accumulator& keys)
{
	auto& self = this->self();

	PageUpdateMgr mgr(self);

	mgr.add(node);

	try {
		NonLeafDispatcher::dispatch(node, UpdateNodeFn(), idx, keys);
		return true;
	}
	catch (PackedOOMException ex)
	{
		mgr.rollback();
		return false;
	}
}

M_PARAMS
void M_TYPE::updatePath(NodeBaseG& node, Int& idx, const Accumulator& keys)
{
	auto& self = this->self();

	if (self.updateNode(node, idx, keys))
	{
		Int size 		= self.getNodeSize(node, 0);
		Int split_idx	= size / 2;

		NodeBaseG right = self.splitPathP(node, split_idx);

		if (idx >= split_idx)
		{
			idx -= split_idx;
			node = right;
		}
	}

	if(!node->is_root())
	{
		Int parent_idx = node->parent_idx();
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePath(parent, parent_idx, keys);
	}
}


M_PARAMS
void M_TYPE::updatePathNoBackup(NodeBaseG& node, Int idx, const Accumulator& keys)
{
	auto& self = this->self();

	self.updateNode(node, idx, keys);

	if(!node->is_root())
	{
		Int parent_idx = node->parent_idx();
		NodeBaseG parent = self.getNodeParent(node, Allocator::UPDATE);

		self.updatePathNoBackup(parent, parent_idx, keys);
	}
}









M_PARAMS
void M_TYPE::updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, std::function<void (Int, Int)> fn)
{
    auto& self = this->self();

	for (Int c = level; c < path.getSize(); c++)
    {
		self.updateCounters(path, c, idx, counters, fn);
        idx = path[c].parent_idx();
    }
}



M_PARAMS
void M_TYPE::updateUpNoBackup(TreePath& path, Int level, Int idx, const Accumulator& counters)
{
	auto& self = this->self();

	for (Int c = level; c < path.getSize(); c++)
	{
		self.updateNodeCounters(path[c], idx, counters);
		idx = path[c].parent_idx();
	}
}


M_PARAMS
void M_TYPE::updateParentIfExists(TreePath& path, Int level, const Accumulator& counters)
{
	auto& self = this->self();
	self.updateUp(path, level + 1, path[level].parent_idx(), counters, [](Int, Int) {});
}


M_PARAMS
bool M_TYPE::updateCounters(
		TreePath& path,
		Int level,
		Int idx,
		const Accumulator& counters,
		std::function<void (Int, Int)> fn
)
{
    auto& self = this->self();
    NodeBaseG& node = path[level];

    node.update();

    PageUpdateMgr mgr(self);
    mgr.add(node);

    try {
    	self.addKeys(path[level], idx, counters, true);
    }
    catch (PackedOOMException ex)
    {
    	mgr.rollback();
    	auto right = path;

    	Int size = self.getNodeSize(node, 0);

    	Int split_idx = size / 2;

    	self.splitPath(path, right, level, Position(split_idx), -1);

    	if (idx >= split_idx)
    	{
    		path = right;
    		idx -= split_idx;

    		fn(level, idx);
    	}

    	self.addKeys(path[level], idx, counters, true);
    }

    return false; //proceed further unconditionally
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

	PageUpdateMgr mgr(self);
	mgr.add(node);

	try {
		NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
	}
	catch (PackedOOMException ex)
	{
		mgr.rollback();
		auto right = path;

		Int size = self.getNodeSize(node, 0);

		Int split_idx = size / 2;

		self.splitPath(path, right, level, Position(split_idx), -1);

		if (idx >= split_idx)
		{
			path = right;
			idx -= split_idx;
		}

		NonLeafDispatcher::dispatch(node, InsertFn(), idx, keys, id);
	}
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

	Accumulator keys = self.splitNode(left_node, other, idx);

	Int parent_idx   = left[level].parent_idx();

	self.updateUpNoBackup(left, level + 1, parent_idx, -keys);

	PageUpdateMgr mgr(self);
	mgr.add(left_parent);

	try {
		self.insertNonLeaf(left_parent, parent_idx + 1, keys, other->id());
		self.updateChildren(left_parent, parent_idx + 1);

		other->parent_id()  = left_parent->id();
		other->parent_idx() = parent_idx + 1;

		self.updateUpNoBackup(left, level + 2, left[level + 1].parent_idx(), keys);

		right[level].node()         = other;
		right[level].parent_idx()   = parent_idx + 1;
	}
	catch (PackedOOMException ex)
	{
		mgr.rollback();

		splitPath(left, right, level + 1, Position(parent_idx + 1), active_streams);

		self.insertNonLeaf(right[level + 1], 0, keys, other->id());
		self.updateChildren(right[level + 1], 1);

		other->parent_id()  = right[level + 1]->id();
		other->parent_idx() = 0;

		self.updateUpNoBackup(right, level + 2, right[level + 1].parent_idx(), keys);

		right[level].node()         = other;
		right[level].parent_idx()   = 0;
	}
}


#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
