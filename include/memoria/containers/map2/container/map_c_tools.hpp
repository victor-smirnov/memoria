
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_C_TOOLS_HPP
#define _MEMORIA_MODELS_IDX_MAP2_C_TOOLS_HPP


#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>
#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrToolsName)

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

	typedef typename Base::WrappedCtr::BalTreeNodeTraits						BalTreeNodeTraits;


	template <typename Node>
	Int getNodeTraitsFn(BalTreeNodeTraits trait, Int page_size) const
	{
		switch (trait)
		{
		case BalTreeNodeTraits::MAX_CHILDREN:
			return Node::Map::max_tree_size(page_size - sizeof(Node) + sizeof(typename Node::Map)); break;

		default: throw DispatchException(MEMORIA_SOURCE, "Unknown static node trait value", (Int)trait);
		}
	}

	MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(GetNodeTraitsFn, getNodeTraitsFn, Int);

	Int getNodeTraitInt(BalTreeNodeTraits trait, bool root, bool leaf) const
	{
		Int page_size = self().ctr().getRootMetadata().page_size();
		return NodeDispatcher::template dispatchStaticRtn<TreeMapNode>(root, leaf, GetNodeTraitsFn(me()), trait, page_size);
	}


    void sumLeafKeys(const NodeBase *node, Int from, Int count, Accumulator& keys) const
    {
    	VectorAdd(keys, LeafDispatcher::dispatchConstRtn(node, typename Base::WrappedCtr::SumKeysFn(&self().ctr()), from, count));
    }

    template <typename Node>
    void setAndReindexFn(Node* node, Int idx, const Element& element) const
    {
    	for (Int c = 0; c < Base::WrappedCtr::Indexes; c++)
    	{
    		node->map().key(c, idx) = std::get<0>(element.first)[c];
    	}

    	node->map().data(idx) = element.second;

    	if (idx == node->children_count() - 1)
    	{
    		node->map().reindexAll(idx, idx + 1);
    	}
    	else {
    		node->map().reindexAll(idx, node->children_count());
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(SetAndReindexFn, setAndReindexFn);


    void setLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
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

    MEMORIA_DECLARE_NODE_FN_RTN(GetLeafKeyFn, key, Key);

    Key getLeafKey(const NodeBaseG& node, Int idx) const
    {
    	return LeafDispatcher::dispatchConstRtn(node.page(), GetLeafKeyFn(), idx);
    }

    MEMORIA_DECLARE_NODE_FN_RTN(GetLeafKeysFn, keys, Accumulator);
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
    	LeafDispatcher::dispatch(node, typename Base::WrappedCtr::AddKeysFn(&self().ctr()), idx, keys, reindex_fully);
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map2::CtrToolsName)
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
    }

    self().ctr().updateUp(path, level + 1, idx, counters, reindex_fully);
}


M_PARAMS
void M_TYPE::makeLeafRoom(TreePath& path, Int start, Int count) const
{
    if (count > 0)
    {
        path[0].node().update();
        LeafDispatcher::dispatch(path[0].node(), typename Base::WrappedCtr::MakeRoomFn(), start, count);
    }
}


#undef M_TYPE
#undef M_PARAMS


}


#endif
