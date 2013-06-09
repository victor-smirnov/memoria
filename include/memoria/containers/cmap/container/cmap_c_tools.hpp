
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_CMAP_CTR_TOOLS_HPP
#define _MEMORIA_CONTAINERS_CMAP_CTR_TOOLS_HPP


#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>
#include <memoria/containers/cmap/cmap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::cmap::CtrToolsName)

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

	typedef typename Base::BalTreeNodeTraits									BalTreeNodeTraits;


	template <typename LeafElement>
	struct SetLeafEntryFn {

		template <Int Idx, typename Tree>
		void stream(Tree* tree, Int idx, const LeafElement& element, Accumulator* delta)
		{
			MEMORIA_ASSERT_TRUE(tree != nullptr);

			auto previous = tree->value(idx);
			tree->value(idx) = std::get<Idx>(element.first)[0];

			std::get<Idx>(*delta)[0] = std::get<Idx>(element.first)[0] - previous;

			tree->reindex();
		}

		template <typename Node>
		void treeNode(Node* node, Int stream, Int idx, const LeafElement& element, Accumulator* delta)
		{
			node->process(stream, *this, idx, element, delta);
		}
	};


	template <typename LeafElement>
	Accumulator setLeafEntry(NodeBaseG& node, Int stream, Int idx, const LeafElement& element) const
	{
		Accumulator delta;

		node.update();
		LeafDispatcher::dispatch(node.page(), SetLeafEntryFn<LeafElement>(), stream, idx, element, &delta);

		return delta;
	}



	template <typename Node>
	Int getNodeTraitsFn(BalTreeNodeTraits trait, Int page_size) const
	{
		switch (trait)
		{
		case BalTreeNodeTraits::MAX_CHILDREN:
			return Node::max_tree_size_for_block(page_size); break;

		default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", (Int)trait);
		}
	}

	MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

	Int getNodeTraitInt(BalTreeNodeTraits trait, bool root, bool leaf) const
	{
		Int page_size = self().getRootMetadata().page_size();
		return NodeDispatcher::template dispatchStaticRtn<TreeMapNode>(root, leaf, GetNodeTraitsFn(&self()), trait, page_size);
	}


    void sumLeafKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	VectorAdd(keys, LeafDispatcher::dispatchConstRtn(node, typename Base::SumKeysFn(&self()), from, count));
    }

    template <typename Node>
    void setAndReindexFn(Node* node, Int idx, const Element& element) const
    {
    	node->value(idx) = element.second;

    	if (idx == node->size(0) - 1)
    	{
    		node->reindexAll(idx, idx + 1);
    	}
    	else {
    		node->reindexAll(idx, node->size(0));
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(SetAndReindexFn, setAndReindexFn);


    void setLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
    	self().setKeys(node, idx, element.first);

    	node.update();
    	LeafDispatcher::dispatch(node.page(), SetAndReindexFn(me()), idx, element);
    }


    template <typename Node>
    Value getLeafDataFn(const Node* node, Int idx) const
    {
    	return node->value(idx);
    }

    MEMORIA_CONST_FN_WRAPPER_RTN(GetLeafDataFn, getLeafDataFn, Value);

    Value getLeafData(const NodeBaseG& node, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node.page(), GetLeafDataFn(me()), idx);
    }




    template <typename Node>
    void setLeafDataFn(Node* node, Int idx, const Value& val) const
    {
    	node->value(idx) = val;
    }

    MEMORIA_CONST_FN_WRAPPER(SetLeafDataFn, setLeafDataFn);



    void setLeafData(NodeBaseG& node, Int idx, const Value &val)
    {
    	node.update();
    	LeafDispatcher::dispatch(node.page(), SetLeafDataFn(me()), idx, val);
    }

    struct GetLeafKeyFn
    {
    	typedef Key ReturnType;
    	typedef Key ResultType;
    	template <typename Node>
    	ReturnType treeNode(const Node* node, Int idx) const
    	{
    		auto* tree = node->tree0();
    		return tree->value(idx);
    	}
    };


    Key getLeafKey(const NodeBaseG& node, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node.page(), GetLeafKeyFn(), idx);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetLeafKeysFn, keysAt, Accumulator);
    Accumulator getLeafKeys(const NodeBaseG& node, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node.page(), GetLeafKeysFn(), idx);
    }


    void makeLeafRoom(TreePath& path, Int start, Int count) const;

    void updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully = false);
    bool updateLeafCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully) const;


    void addLeafKeys(NodeBaseG& node, int idx, const Accumulator& keys, bool reindex_fully = false) const
    {
    	node.update();
    	LeafDispatcher::dispatch(node, typename Base::AddKeysFn(&self()), idx, keys, reindex_fully);
    }


    MEMORIA_DECLARE_NODE_FN(LayoutNodeFn, layout);
    void layoutNode(NodeBaseG& node, UBigInt active_streams) const
    {
    	NodeDispatcher::dispatch(node, LayoutNodeFn(), active_streams);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetStreamCapacityFn, capacity, Int);

    Int getStreamCapacity(const NodeBaseG& node, Int stream) const
    {
    	Position reservation;
        return getStreamCapacity(node, reservation, stream);
    }

    Int getStreamCapacity(const NodeBaseG& node, const Position& reservation, Int stream) const
    {
    	return LeafDispatcher::dispatchConstRtn(node, GetStreamCapacityFn());
    }

    void initLeaf(NodeBaseG& node) const
    {
    	node.update();
    	self().layoutNode(node, 1);
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::cmap::CtrToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::updateLeafCounters(NodeBaseG& node, Int idx, const Accumulator& counters, bool reindex_fully) const
{
    node.update();
    self().addLeafKeys(node, idx, counters, reindex_fully);

    return false; //proceed further unconditionally
}


M_PARAMS
void M_TYPE::updateUp(TreePath& path, Int level, Int idx, const Accumulator& counters, bool reindex_fully)
{
    if (level == 0)
    {
    	if (self().updateLeafCounters(path[level].node(), idx, counters, reindex_fully))
    	{
    		return;
    	}
    	else {
    		idx = path[level].parent_idx();
    	}

    	Base::updateUp(path, level + 1, idx, counters, reindex_fully);
    }
    else {
    	Base::updateUp(path, level, idx, counters, reindex_fully);
    }


}


M_PARAMS
void M_TYPE::makeLeafRoom(TreePath& path, Int start, Int count) const
{
    if (count > 0)
    {
        path[0].node().update();
        LeafDispatcher::dispatch(path[0].node(), typename Base::MakeRoomFn(), start, count);
    }
}


#undef M_TYPE
#undef M_PARAMS


}


#endif
