
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
	typedef typename Base::TreePath                                             TreePath;

    bool NextKey();
    bool HasNextKey();

    bool PrevKey();

    bool HasPrevKey();

    bool NextLeaf();
    bool HasNextLeaf();

    bool PrevLeaf();
    bool HasPrevLeaf();


    void Init() {
    	Base::Init();
    }

    bool IsFound() {
        return (!me()->IsEnd()) && me()->IsNotEmpty();
    }

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
			me()->KeyNum()++;

			me()->model().FinishPathStep(me()->path(), me()->key_idx());

			me()->ReHash();

			return true;
		}
		else {
			bool has_next_leaf = me()->NextLeaf();
			if (has_next_leaf)
			{
				me()->key_idx() = 0;
			}
			else {
				me()->key_idx() = me()->page()->children_count();

				me()->model().FinishPathStep(me()->path(), me()->key_idx());
			}

			me()->KeyNum()++;

			me()->ReHash();

			return has_next_leaf;
		}
	}
	else {
		return false;
	}
}

M_PARAMS
bool M_TYPE::HasNextKey()
{
	if (!me()->IsEnd())
	{
		if (me()->key_idx() < me()->page()->children_count() - 1)
		{
			return true;
		}
		else {
			return me()->HasNextLeaf();
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
		me()->KeyNum()--;

		me()->model().FinishPathStep(me()->path(), me()->key_idx());

		me()->ReHash();

		return true;
	}
	else {
		bool has_prev_leaf = me()->PrevLeaf();

		if (has_prev_leaf)
		{
			me()->key_idx() = me()->page()->children_count() - 1;
			me()->KeyNum()--;
		}
		else {
			me()->key_idx() = -1;

			me()->model().FinishPathStep(me()->path(), me()->key_idx());
		}

		me()->ReHash();

		return has_prev_leaf;
	}
}


M_PARAMS
bool M_TYPE::HasPrevKey()
{
	if (me()->key_idx() > 0)
	{
		return true;
	}
	else {
		return me()->HasPrevLeaf();
	}
}



//FIXME: Should NextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::NextLeaf()
{
	if (me()->model().GetNextNode(me()->path()))
	{
		// FIXME: KeyNum ?

		me()->key_idx() = 0;

		return true;
	}

	return false;
}


M_PARAMS
bool M_TYPE::HasNextLeaf()
{
	TreePath path = me()->path();
	return me()->model().GetNextNode(path);
}


M_PARAMS
bool M_TYPE::PrevLeaf()
{
	if (me()->model().GetPrevNode(me()->path()))
	{
		// FIXME: KeyNum

		me()->key_idx() = me()->page()->children_count() - 1;

		return true;
	}
	return false;
}

M_PARAMS
bool M_TYPE::HasPrevLeaf()
{
	TreePath path = me()->path();
	return me()->model().GetPrevNode(path);
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
