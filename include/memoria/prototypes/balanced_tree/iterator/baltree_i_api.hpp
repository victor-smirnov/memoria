
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_API_H

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/balanced_tree/baltree_names.hpp>
#include <memoria/core/container/macros.hpp>


#include <iostream>


namespace memoria    {

using namespace memoria::balanced_tree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::balanced_tree::IteratorAPIName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Element                                   Element;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;

    bool nextKey();
    bool hasNextKey();

    bool prevKey();

    bool hasPrevKey();

    bool nextLeaf();
    bool hasNextLeaf();

    bool prevLeaf();
    bool hasPrevLeaf();

    bool next() {
        return me()->nextKey();
    }

    bool prev() {
        return me()->prevKey();
    }

    BigInt skipFw(BigInt amount);
    BigInt skipBw(BigInt amount);
    BigInt skip(BigInt amount);

    bool operator++() {
        return me()->nextKey();
    }

    bool operator--() {
        return me()->prevKey();
    }

    bool operator++(int) {
        return me()->nextKey();
    }

    bool operator--(int) {
        return me()->prevKey();
    }

    BigInt operator+=(BigInt size)
    {
        return me()->skipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
        return me()->skipBw(size);
    }

    operator Value () const
    {
        return me()->getValue();
    }

    Value value() const
    {
        return me()->getValue();
    }

    Accumulator keys() const
    {
        return me()->getKeys();
    }

    Key key() const
    {
        return me()->getKey(0);
    }

    Key key(Int key_num) const
    {
        return me()->getKey(key_num);
    }

    MyType& operator<<(const Element& element)
    {
        me()->model().insert(*me(), element);
        return *me();
    }

    MyType& operator*()
    {
        return *me();
    }

    void remove()
    {
        Accumulator keys;
        me()->model().removeEntry(*me(), keys);
    }

    Value getValue() const
    {
        return me()->model().getLeafData(me()->page(), me()->key_idx());
    }

    void setData(const Value& data)
    {
        if (!me()->isEnd())
        {
            me()->model().setLeafData(me()->leaf().node(), me()->key_idx(), data);
        }
        else {
            throw Exception(MEMORIA_SOURCE, "insertion after the end of iterator");
        }
    }

    Key getRawKey(Int keyNum) const
    {
        return me()->model().getKey(me()->page(), keyNum, me()->key_idx());
    }

    Accumulator getRawKeys() const
    {
        return me()->model().getKeys(me()->page(), me()->key_idx());
    }

    void updateUp(const Accumulator& keys)
    {
        me()->model().updateUp(me()->path(), 0, me()->key_idx(), keys);
    }

//    void init() {
//        Base::init();
//    }

    bool IsFound() {
        return (!me()->isEnd()) && me()->isNotEmpty();
    }

    template <typename Walker>
    BigInt skip_keys_fw(BigInt distance)
    {
        if (me()->page().isEmpty())
        {
            return 0;
        }
        else if (me()->key_idx() + distance < me()->page()->children_count())
        {
            me()->key_idx() += distance;
        }
        else {
            Walker walker(distance, me()->model());

            if (me()->model().walkFw(me()->path(), me()->key_idx(), walker))
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

            if (me()->model().walkBw(me()->path(), me()->key_idx(), walker))
            {
                me()->key_idx() = -1;
                me()->ReHash();
                return walker.sum();
            }
        }

        me()->ReHash();
        return distance;
    }


    Key getKey(Int i) const
    {
        return me()->getRawKey(i) + me()->prefixes()[i];
    }

    Accumulator getKeys() const
    {
        return me()->getRawKeys() + me()->prefixes();
    }

    void ComputePrefix(Accumulator& pfx)
    {
        compute_base(pfx);
    }


    void ComputeBase()
    {
    }

    void dumpKeys(ostream& out)
    {
        Base::dumpKeys(out);

        out<<"Prefix:  "<<me()->prefixes()<<endl;
    }

private:
    void compute_base(Accumulator& accum)
    {
    	TreePath&   path0 = me()->path();
    	Int         idx   = me()->key_idx();

    	for (Int c = 0; c < path0.getSize(); c++)
    	{
    		me()->model().sumKeys(path0[c].node(), 0, idx, accum);
    		idx = path0[c].parent_idx();
    	}
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::balanced_tree::IteratorAPIName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

// --------------------- PUBLIC API --------------------------------------


M_PARAMS
BigInt M_TYPE::skipFw(BigInt amount)
{
    BigInt cnt = 0;

    for (BigInt c = 0; c < amount; c++, cnt++)
    {
        if (!me()->nextKey())
        {
            break;
        }
    }

    return cnt;
}


M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
    BigInt cnt = 0;

    for (BigInt c = 0; c < amount; c++, cnt++)
    {
        if (!me()->prevKey())
        {
            break;
        }
    }

    return cnt;
}


M_PARAMS
BigInt M_TYPE::skip(BigInt amount)
{
    if (amount >= 0)
    {
        return me()->skipFw(amount);
    }
    else {
        return me()->skipBw(-amount);
    }
}


M_PARAMS
bool M_TYPE::nextKey()
{
    if (!me()->isEnd())
    {
        if (me()->key_idx() < me()->page()->children_count() - 1)
        {
            me()->cache().Prepare();

            me()->key_idx()++;

            me()->keyNum()++;

            me()->model().finishPathStep(me()->path(), me()->key_idx());

            me()->cache().nextKey(false);

            return true;
        }
        else {
            me()->cache().Prepare();

            bool has_next_leaf = me()->nextLeaf();
            if (has_next_leaf)
            {
                me()->key_idx() = 0;

                me()->cache().nextKey(false);
            }
            else {
                me()->key_idx() = me()->page()->children_count();

                me()->model().finishPathStep(me()->path(), me()->key_idx());

                me()->cache().nextKey(true);
            }

            me()->keyNum()++;

            return has_next_leaf;
        }
    }
    else {
        return false;
    }
}

M_PARAMS
bool M_TYPE::hasNextKey()
{
    if (!me()->isEnd())
    {
        if (me()->key_idx() < me()->page()->children_count() - 1)
        {
            return true;
        }
        else {
            return me()->hasNextLeaf();
        }
    }
    else {
        return false;
    }
}



M_PARAMS
bool M_TYPE::prevKey()
{
    if (me()->key_idx() > 0)
    {
        me()->key_idx()--;
        me()->keyNum()--;

        me()->model().finishPathStep(me()->path(), me()->key_idx());

        me()->cache().Prepare();
        me()->cache().prevKey(false);

        return true;
    }
    else {
        bool has_prev_leaf = me()->prevLeaf();

        if (has_prev_leaf)
        {
            me()->key_idx() = me()->page()->children_count() - 1;
            me()->keyNum()--;

            me()->cache().Prepare();
            me()->cache().prevKey(false);
        }
        else {
            me()->key_idx() = -1;

            me()->model().finishPathStep(me()->path(), me()->key_idx());

            me()->cache().Prepare();
            me()->cache().prevKey(true);
        }

        return has_prev_leaf;
    }
}


M_PARAMS
bool M_TYPE::hasPrevKey()
{
    if (me()->key_idx() > 0)
    {
        return true;
    }
    else {
        return me()->hasPrevLeaf();
    }
}



//FIXME: Should nextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::nextLeaf()
{
    if (me()->model().getNextNode(me()->path()))
    {
        // FIXME: keyNum ?

        me()->key_idx() = 0;

        return true;
    }

    return false;
}


M_PARAMS
bool M_TYPE::hasNextLeaf()
{
    TreePath path = me()->path();
    return me()->model().getNextNode(path);
}


M_PARAMS
bool M_TYPE::prevLeaf()
{
    if (me()->model().getPrevNode(me()->path()))
    {
        // FIXME: keyNum

        me()->key_idx() = me()->page()->children_count() - 1;

        return true;
    }
    return false;
}

M_PARAMS
bool M_TYPE::hasPrevLeaf()
{
    TreePath path = me()->path();
    return me()->model().getPrevNode(path);
}



#undef M_TYPE
#undef M_PARAMS


}


#endif
