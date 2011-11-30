
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

    typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;

    bool NextKey();

    bool PrevKey();

    bool NextLeaf();

    bool PrevLeaf() ;


    NodeBaseG GetNextNode(NodeBase* page);

    NodeBaseG GetNextNode()
    {
    	return me_.GetNextNode(me_.page());
    }

    NodeBaseG GetPrevNode(NodeBase* page);
    NodeBaseG GetPrevNode()
    {
    	return me_.GetPrevNode(me_.page());
    }

    void Init() {
    	Base::Init();
    }

    bool IsFound() {
        return (!me_.IsEnd()) && me_.IsNotEmpty();
    }



private:

    NodeBaseG __get_next_node(NodeBase* page, Int &idx1, Int level);

    NodeBaseG __get_prev_node(NodeBase* page, Int &idx1, Int level);

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::btree::IteratorAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------

M_PARAMS
bool M_TYPE::NextKey()
{
	MEMORIA_TRACE(me_, "NextKey");
	if (!me_.IsEnd())
	{
		if (me_.key_idx() < me_.GetChildrenCount(me_.page()) - 1)
		{
			me_.key_idx()++;
			me_.ReHash();
			return true;
		}
		else {
			bool val = me_.NextLeaf();
			if (val) {
				me_.key_idx() = 0;
			}
			else {
				me_.key_idx() = me_.GetChildrenCount(me_.page());
			}

			me_.ReHash();
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
	MEMORIA_TRACE(me_, "PrevKey");
	if (me_.key_idx() > 0)
	{
		me_.key_idx()--;
		me_.ReHash();
		return true;
	}
	else {
		bool val = me_.PrevLeaf();

		if (val) {
			me_.key_idx() = me_.GetChildrenCount(me_.page()) - 1;
		}
		else {
			me_.key_idx() = -1;
		}

		me_.ReHash();
		return val;
	}
}

//FIXME: Should NextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::NextLeaf()
{
	MEMORIA_TRACE(me_, "NextLeaf");
	NodeBaseG node = me_.GetNextNode(me_.page());
	if (node != NULL)
	{
		me_.key_idx() = 0;
		me_.page() = node;
		return true;
	}
	return false;
}

M_PARAMS
bool M_TYPE::PrevLeaf()
{
	MEMORIA_TRACE(me_, "PrevLeaf");
	NodeBaseG node = me_.GetPrevNode(me_.page());
	if (node != NULL)
	{
		me_.page() = node;
		me_.key_idx() = me_.GetChildrenCount(me_.page()) - 1;
		return true;
	}
	return false;
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::GetNextNode(NodeBase* page)
{
	if (page->is_root())
	{
		return NodeBaseG(&me_.model().allocator());
	}
	else {
		Int parent_idx = page->parent_idx();
		NodeBaseG parent = me_.GetParent(page);
		if (parent == NULL)
		{
			throw NullPointerException(MEMORIA_SOURCE, "Parent must not be null");
		}
		return me_.__get_next_node(parent, parent_idx, page->level());
	}
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::GetPrevNode(NodeBase* page)
{
	if (page->is_root()) {
		return NodeBaseG(&me_.model().allocator());
	}
	else {
		Int parent_idx = page->parent_idx();
		NodeBaseG parent = me_.GetParent(page);
		if (parent == NULL) throw NullPointerException(MEMORIA_SOURCE, "Parent must not be null");
		return me_.__get_prev_node(parent, parent_idx, page->level());
	}
}





// ------------------------------------ PRIVATE API --------------------------------

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::__get_next_node(NodeBase* page, Int &idx1, Int level)
{
	if (idx1 < me_.GetChildrenCount(page) - 1)
	{
		NodeBaseG page0 = me_.GetChild(page, idx1 + 1);

		while (page0->level() != level)
		{
			page0 = me_.GetChild(page0, 0);
		}

		idx1++;
		return page0;
	}
	else {
		if (!page->is_root())
		{
			NodeBaseG parent = me_.GetParent(page);
			Int idx0 = page->parent_idx();
			return __get_next_node(parent, idx0, level);
		}
	}

	return NodeBaseG(&me_.model().allocator());
}

M_PARAMS
typename M_TYPE::NodeBaseG M_TYPE::__get_prev_node(NodeBase* page, Int &idx1, Int level)
{
	if (idx1 > 0)
	{
		NodeBaseG page0 = me_.GetChild(page, idx1 - 1);

		while (page0->level() != level) {
			page0 = me_.GetLastChild(page0);
		}

		--idx1;
		return page0;
	}
	else {
		if (!page->is_root())
		{
			NodeBaseG parent = me_.GetParent(page);
			Int idx0 = page->parent_idx();
			return __get_prev_node(parent, idx0, level);
		}
	}

	return NodeBaseG(&me_.model().allocator());
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
