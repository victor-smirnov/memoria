
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_API_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_API_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models/logs.hpp>

#include <memoria/prototypes/btree/names.hpp>
#include <memoria/core/container/macros.hpp>





namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorAPIName)

	typedef typename Base::Allocator											Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;

    bool NextKey();

    bool PrevKey();

    bool NextLeaf();

    bool PrevLeaf() ;


    NodeBaseG GetNextNode(NodeBase* page);

    NodeBaseG GetNextNode()
    {
    	return me()->GetNextNode(me()->page());
    }

    NodeBaseG GetPrevNode(NodeBase* page);
    NodeBaseG GetPrevNode()
    {
    	return me()->GetPrevNode(me()->page());
    }

    void Init() {
    	Base::Init();
    }

    bool IsFound() {
        return (!me()->IsEnd()) && me()->IsNotEmpty();
    }



private:

    NodeBaseG __get_next_node(NodeBaseG& page, Int &idx1, Int level);

    NodeBaseG __get_prev_node(NodeBaseG& page, Int &idx1, Int level);

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::btree::IteratorAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------

M_PARAMS
bool M_TYPE::NextKey()
{
	if (!me()->IsEnd())
	{
		if (me()->key_idx() < me()->page()->children_count() - 1)
		{
			me()->key_idx()++;
			me()->ReHash();
			return true;
		}
		else {
			bool val = me()->NextLeaf();
			if (val) {
				me()->key_idx() = 0;
			}
			else {
				me()->key_idx() = me()->page()->children_count();
			}

			me()->ReHash();
			return val;
		}
	}
	else {
		return false;
	}
}

M_PARAMS
bool M_TYPE::PrevKey()
{
	if (me()->key_idx() > 0)
	{
		me()->key_idx()--;
		me()->ReHash();
		return true;
	}
	else {
		bool val = me()->PrevLeaf();

		if (val) {
			me()->key_idx() = me()->page()->children_count() - 1;
		}
		else {
			me()->key_idx() = -1;
		}

		me()->ReHash();
		return val;
	}
}

//FIXME: Should NextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::NextLeaf()
{
	NodeBaseG node = me()->GetNextNode(me()->page());
	if (node != NULL)
	{
		me()->key_idx() = 0;
		me()->page() = node;
		return true;
	}
	return false;
}

M_PARAMS
bool M_TYPE::PrevLeaf()
{
	NodeBaseG node = me()->GetPrevNode(me()->page());
	if (node != NULL)
	{
		me()->page() = node;
		me()->key_idx() = me()->page()->children_count() - 1;
		return true;
	}
	return false;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::GetNextNode(NodeBase* page)
{
	if (page->is_root())
	{
		return NodeBaseG();
	}
	else {
		Int parent_idx = page->parent_idx();
		NodeBaseG parent = me()->GetParent(page, Allocator::READ);
		if (parent == NULL)
		{
			throw NullPointerException(MEMORIA_SOURCE, "Parent must not be null");
		}
		return me()->__get_next_node(parent, parent_idx, page->level());
	}
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::GetPrevNode(NodeBase* page)
{
	if (page->is_root()) {
		return NodeBaseG();
	}
	else {
		Int parent_idx = page->parent_idx();
		NodeBaseG parent = me()->GetParent(page, Allocator::READ);
		if (parent == NULL) throw NullPointerException(MEMORIA_SOURCE, "Parent must not be null");
		return me()->__get_prev_node(parent, parent_idx, page->level());
	}
}





// ------------------------------------ PRIVATE API --------------------------------

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::__get_next_node(NodeBaseG& page, Int &idx1, Int level)
{
	if (idx1 < page->children_count() - 1)
	{
		NodeBaseG page0 = me()->GetChild(page, idx1 + 1, Allocator::READ);

		while (page0->level() != level)
		{
			page0 = me()->GetChild(page0, 0, Allocator::READ);
		}

		idx1++;
		return page0;
	}
	else {
		if (!page->is_root())
		{
			NodeBaseG parent = me()->GetParent(page, Allocator::READ);
			Int idx0 = page->parent_idx();
			return __get_next_node(parent, idx0, level);
		}
	}

	return NodeBaseG();
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::__get_prev_node(NodeBaseG& page, Int &idx1, Int level)
{
	if (idx1 > 0)
	{
		NodeBaseG page0 = me()->GetChild(page, idx1 - 1, Allocator::READ);

		while (page0->level() != level) {
			page0 = me()->GetLastChild(page0, Allocator::READ);
		}

		--idx1;
		return page0;
	}
	else {
		if (!page->is_root())
		{
			NodeBaseG parent = me()->GetParent(page, Allocator::READ);
			Int idx0 = page->parent_idx();
			return __get_prev_node(parent, idx0, level);
		}
	}

	return NodeBaseG();
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
