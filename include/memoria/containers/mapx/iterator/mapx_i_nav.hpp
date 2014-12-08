
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP
#define _MEMORIA_CONTAINERS_MAPX_ITER_NAV_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::mapx::ItrNavName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Base::Container::Value                                     Value;
    typedef typename Base::Container::Key                                       Key;
    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;

    bool operator++() {
        return self().nextKey();
    }

    bool operator--() {
        return self().prevKey();
    }

    bool operator++(int) {
        return self().nextKey();
    }

    bool operator--(int) {
        return self().prevKey();
    }

    BigInt operator+=(BigInt size)
    {
        return self().skipFw(size);
    }

    BigInt operator-=(BigInt size)
    {
        return self().skipBw(size);
    }

    Key getRawKey() const
    {
        auto& self = this->self();
        return self.raw_key();
    }

    Accumulator getRawKeys() const
    {
        auto& self = this->self();

        Accumulator accum;

        std::get<0>(accum)[0] = self.raw_key();

        return accum;
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

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::map::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


//FIXME: Should nextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::nextLeaf()
{
    auto& self = this->self();

    auto next = self.ctr().getNextNodeP(self.leaf());

    if (next)
    {
        self.leaf() = next;
        self.idx()  = 0;
        return true;
    }

    return false;
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
    auto& self = this->self();

    auto prev = self.ctr().getPrevNodeP(self.leaf());

    if (prev)
    {
        self.leaf() = prev;
        self.idx()  = self.leafSize(self.stream()) - 1;

        return true;
    }

    return false;
}



M_PARAMS
bool M_TYPE::nextKey()
{
    auto& self = this->self();
    auto& ctr  = self.ctr();

    if (!self.isEnd())
    {
        if (self.idx() < ctr.getNodeSize(self.leaf(), 0) - 1)
        {
            self.cache().Prepare();

            self.idx()++;

            self.keyNum()++;

            self.cache().nextKey(false);

            return true;
        }
        else {
            self.cache().Prepare();

            bool has_next_leaf = self.nextLeaf();
            if (has_next_leaf)
            {
                self.idx() = 0;

                self.cache().nextKey(false);
            }
            else {
                self.idx() = ctr.getNodeSize(self.leaf(), 0);

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
        if (self.idx() < ctr.getNodeSize(self.leaf(), 0) - 1)
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
        self.idx()--;
        self.keyNum()--;

        self.cache().Prepare();
        self.cache().prevKey(false);

        return true;
    }
    else {
        bool has_prev_leaf = self.prevLeaf();

        if (has_prev_leaf)
        {
            self.idx() = ctr.getNodeSize(self.leaf(), 0) - 1;
            self.keyNum()--;

            self.cache().Prepare();
            self.cache().prevKey(false);
        }
        else {
            self.idx() = -1;

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
