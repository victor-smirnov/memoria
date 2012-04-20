
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_API_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_API_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/logs.hpp>

#include <memoria/prototypes/btree/names.hpp>
#include <memoria/core/container/macros.hpp>





namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorAPIName)

	typedef typename Base::Allocator											Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Base::Container::Value                                     Value;
	typedef typename Base::Container::Key                                       Key;
	typedef typename Base::Container::Element                                   Element;
	typedef typename Base::Container::Accumulator                               Accumulator;
	typedef typename Base::Container                                            Container;

    bool NextKey();
    bool HasNextKey();

    bool PrevKey();

    bool HasPrevKey();

    bool NextLeaf();
    bool HasNextLeaf();

    bool PrevLeaf();
    bool HasPrevLeaf();

    bool Next() {
    	return me()->NextKey();
    }

    bool Prev() {
    	return me()->PrevKey();
    }

    BigInt SkipFw(BigInt amount);
    BigInt SkipBw(BigInt amount);
    BigInt Skip(BigInt amount);

    bool operator++() {
    	return me()->NextKey();
    }

    bool operator--() {
    	return me()->PrevKey();
    }

    bool operator++(int) {
    	return me()->NextKey();
    }

    bool operator--(int) {
    	return me()->PrevKey();
    }

    BigInt operator+=(BigInt size)
    {
    	return me()->SkipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
    	return me()->SkipBw(size);
    }

    operator Value () const
    {
    	return me()->GetValue();
    }

    Value value() const
    {
    	return me()->GetValue();
    }

    Accumulator keys() const
    {
    	return me()->GetKeys();
    }

    Key key() const
    {
    	return me()->GetKey(0);
    }

    Key key(Int key_num) const
    {
    	return me()->GetKey(key_num);
    }

    MyType& operator<<(const Element& element)
    {
    	me()->model().Insert(*me(), element);
    	return *me();
    }

    MyType& operator*()
    {
    	return *me();
    }

    void Remove()
    {
    	Accumulator keys;
    	me()->model().RemoveEntry(*me(), keys);
    }

    Value GetValue() const
    {
    	return me()->model().GetLeafData(me()->page(), me()->key_idx());
    }

    void SetData(const Value& data)
    {
    	if (!me()->IsEnd())
    	{
    		me()->model().SetLeafData(me()->leaf().node(), me()->key_idx(), data);
    	}
    	else {
    		throw MemoriaException(MEMORIA_SOURCE, "Insertion after the end of iterator");
    	}
    }

    Key GetKey(Int keyNum) const
    {
    	return me()->model().GetKey(me()->page(), keyNum, me()->key_idx());
    }

    Accumulator GetKeys() const
    {
    	return me()->model().GetKeys(me()->page(), me()->key_idx());
    }

    void UpdateUp(const Accumulator& keys)
    {
    	me()->model().UpdateUp(me()->path(), 0, me()->key_idx(), keys);
    }

    void Init() {
    	Base::Init();
    }

    bool IsFound() {
        return (!me()->IsEnd()) && me()->IsNotEmpty();
    }

    template <typename Walker>
    BigInt skip_keys_fw(BigInt distance)
    {
    	if (me()->page().is_empty())
    	{
    		return 0;
    	}
    	else if (me()->key_idx() + distance < me()->page()->children_count())
    	{
    		me()->key_idx() += distance;
    	}
    	else {
    		Walker walker(distance, me()->model());

    		if (me()->model().WalkFw(me()->path(), me()->key_idx(), walker))
    		{
    			me()->key_idx()++;
    			me()->ReHash();
    			return walker.sum();
    		}
    	}

    	me()->ReHash();
    	return distance;
    }


    template <typename Walker>
    BigInt skip_keys_bw(BigInt distance)
    {
    	if (me()->page() == NULL)
    	{
    		return 0;
    	}
    	else if (me()->key_idx() - distance >= 0)
    	{
    		me()->key_idx() -= distance;
    	}
    	else {
    		Walker walker(distance, me()->model());

    		if (me()->model().WalkBw(me()->path(), me()->key_idx(), walker))
    		{
    			me()->key_idx() = -1;
    			me()->ReHash();
    			return walker.sum();
    		}
    	}

    	me()->ReHash();
    	return distance;
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE 		MEMORIA_ITERATOR_TYPE(memoria::btree::IteratorAPIName)
#define M_PARAMS 	MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
BigInt M_TYPE::SkipFw(BigInt amount)
{
	BigInt cnt = 0;

	for (BigInt c = 0; c < amount; c++, cnt++)
	{
		if (!me()->NextKey())
		{
			break;
		}
	}

	return cnt;
}


M_PARAMS
BigInt M_TYPE::SkipBw(BigInt amount)
{
	BigInt cnt = 0;

	for (BigInt c = 0; c < amount; c++, cnt++)
	{
		if (!me()->PrevKey())
		{
			break;
		}
	}

	return cnt;
}


M_PARAMS
BigInt M_TYPE::Skip(BigInt amount)
{
	if (amount >= 0)
	{
		return me()->SkipFw(amount);
	}
	else {
		return me()->SkipBw(-amount);
	}
}


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
