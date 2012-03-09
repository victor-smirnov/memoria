
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
			me()->ReHash();

			return true;
		}
		else {
			bool val = me()->NextLeaf();
			if (val)
			{
				me()->key_idx() = 0;
			}
			else {
				me()->key_idx() = me()->page()->children_count();
			}

			me()->KeyNum()++;

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
		me()->KeyNum()--;
		me()->ReHash();

		return true;
	}
	else {
		bool val = me()->PrevLeaf();

		if (val) {
			me()->key_idx() = me()->page()->children_count() - 1;
			me()->KeyNum()--;
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
	if (me()->model().GetNextNode(me()->path()))
	{
		// FIXME: KeyNum

		me()->key_idx() = 0;

		return true;
	}
	return false;
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


#undef M_TYPE
#undef M_PARAMS


}


#endif
