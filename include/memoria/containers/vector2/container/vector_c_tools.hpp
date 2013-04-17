
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTOR2_C_TOOLS_HPP
#define _MEMORIA_CONTAINER_VECTOR2_C_TOOLS_HPP


#include <memoria/prototypes/balanced_tree/baltree_macros.hpp>
#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector2::CtrToolsName)

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
    	node->value(idx) = element.second;

    	if (idx == node->children_count() - 1)
    	{
    		node->reindexAll(idx, idx + 1);
    	}
    	else {
    		node->reindexAll(idx, node->children_count());
    	}
    }

    MEMORIA_CONST_FN_WRAPPER(SetAndReindexFn, setAndReindexFn);


    void setLeafDataAndReindex(NodeBaseG& node, Int idx, const Element& element) const
    {
    	self().ctr().setKeys(node, idx, element.first);

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





MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector2::CtrToolsName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS


}


#endif
