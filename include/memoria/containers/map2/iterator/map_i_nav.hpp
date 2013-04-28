
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_I_NAV_HPP
#define _MEMORIA_MODELS_IDX_MAP2_I_NAV_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::map2::ItrNavName)

	typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Element                                   Element;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;

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


//    operator Value () const
//    {
//        return self.getValue();
//    }
//
//    Value value() const
//    {
//        return self.getValue();
//    }
//
//    Accumulator keys() const
//    {
//        return self.getKeys();
//    }
//
//    Key key() const
//    {
//        return self.getKey(0);
//    }
//
//    Key key(Int key_num) const
//    {
//        return self.getKey(key_num);
//    }
//
//    MyType& operator<<(const Element& element)
//    {
//        self.model().insert(*me(), element);
//        return *me();
//    }

//    MyType& operator*()
//    {
//        return *me();
//    }



//    Value getValue() const
//    {
//        return self.model().getLeafData(self.page(), self.key_idx());
//    }
//
//    void setData(const Value& data)
//    {
//        if (!self.isEnd())
//        {
//            self.model().setLeafData(self.leaf().node(), self.key_idx(), data);
//        }
//        else {
//            throw Exception(MEMORIA_SOURCE, "insertion after the end of iterator");
//        }
//    }

    Key getRawKey() const
    {
    	auto& self = this->self();
        return self.model().getLeafKey(self.leaf(), self.key_idx());
    }

    Accumulator getRawKeys() const
    {
    	auto& self = this->self();
    	return self.model().getLeafKeys(self.leaf(), self.key_idx());
    }

    bool nextKey();
    bool prevKey();

    bool nextLeaf();
    bool prevLeaf();


    bool hasNextKey();
    bool hasPrevKey();

    BigInt skipFw(BigInt amount);
    BigInt skipBw(BigInt amount);
    BigInt skip(BigInt amount);

    BigInt skipStreamFw(Int stream, BigInt distance) {
    	return skipFw(distance);
    }

    BigInt skipStreamBw(Int stream, BigInt distance) {
    	return skipBw(distance);
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::map2::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


//FIXME: Should nextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::nextLeaf()
{
    auto& self = this->self();

	if (self.model().getNextNode(self.path()))
    {
        // FIXME: keyNum ?

        self.key_idx() = 0;

        return true;
    }

    return false;
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
    auto& self = this->self();

    if (self.model().getPrevNode(self.path()))
    {
        self.key_idx() = self.leafSize(self.stream()) - 1;

        return true;
    }
    return false;
}



M_PARAMS
bool M_TYPE::nextKey()
{
    auto& self = this->self();
    auto& ctr  = self.model();

	if (!self.isEnd())
    {
        if (self.key_idx() < ctr.getNodeSize(self.page(), 0) - 1)
        {
            self.cache().Prepare();

            self.key_idx()++;

            self.keyNum()++;

            self.cache().nextKey(false);

            return true;
        }
        else {
            self.cache().Prepare();

            bool has_next_leaf = self.nextLeaf();
            if (has_next_leaf)
            {
                self.key_idx() = 0;

                self.cache().nextKey(false);
            }
            else {
                self.key_idx() = ctr.getNodeSize(self.page(), 0);

                self.cache().nextKey(true);
            }

            self.keyNum()++;

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
	auto& self = this->self();
	auto& ctr  = self.model();

	if (!self.isEnd())
    {
        if (self.key_idx() < ctr.getNodeSize(self.page(), 0) - 1)
        {
            return true;
        }
        else {
            return self.hasNextLeaf();
        }
    }
    else {
        return false;
    }
}



M_PARAMS
bool M_TYPE::prevKey()
{
	auto& self = this->self();
    auto& ctr  = self.model();

    if (self.key_idx() > 0)
    {
        self.key_idx()--;
        self.keyNum()--;

        self.cache().Prepare();
        self.cache().prevKey(false);

        return true;
    }
    else {
        bool has_prev_leaf = self.prevLeaf();

        if (has_prev_leaf)
        {
            self.key_idx() = ctr.getNodeSize(self.page(), 0) - 1;
            self.keyNum()--;

            self.cache().Prepare();
            self.cache().prevKey(false);
        }
        else {
            self.key_idx() = -1;

            self.cache().Prepare();
            self.cache().prevKey(true);
        }

        return has_prev_leaf;
    }
}


M_PARAMS
bool M_TYPE::hasPrevKey()
{
	auto& self = this->self();

    if (self.key_idx() > 0)
    {
        return true;
    }
    else {
        return self.hasPrevLeaf();
    }
}


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
    auto& self = this->self();

	if (amount > 0)
    {
        return self.skipFw(amount);
    }
    else if (amount < 0) {
        return self.skipBw(-amount);
    }
    else {
    	return 0;
    }
}




}

#undef M_TYPE
#undef M_PARAMS


#endif
