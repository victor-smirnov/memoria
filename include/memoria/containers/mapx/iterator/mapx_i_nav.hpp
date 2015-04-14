
// Copyright Victor Smirnov 2014-2015.
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
    typedef typename Base::Container::Position                                  Position;

    bool operator++() {
        return self().nextKey();
    }

    bool operator--() {
        return self().skipBw(1);
    }

    bool operator++(int) {
        return self().skipFw(1);
    }

    bool operator--(int) {
        return self().skipBw(1);
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

    void split()
    {
    	auto& self = this->self();

    	NodeBaseG& leaf = self.leaf();
    	Int& idx        = self.idx();

    	Int size        = self.leaf_size(0);
    	Int split_idx   = size/2;

    	auto right = self.ctr().splitLeafP(leaf, Position::create(0, split_idx));

    	if (idx > split_idx)
    	{
    		leaf = right;
    		idx -= split_idx;

//    		self.updatePrefix();
    	}
    }

    void insert(BigInt key, BigInt value)
    {
    	auto& self = this->self();

    	self.ctr().template insertStreamEntry<0>(self, std::make_tuple(core::StaticVector<BigInt, 1>({key}), value));
    }

    void remove() {
    	auto& self = this->self();
    	self.ctr().template removeStreamEntry<0>(self);
    }

    template <typename WTypes, typename LeafPath>
    using GTFWWalker = memoria::bt1::FindGTForwardWalker2<memoria::bt1::WalkerTypes<WTypes, LeafPath>>;

    template <typename WTypes, typename LeafPath>
    using GEFWWalker = memoria::bt1::FindGEForwardWalker2<memoria::bt1::WalkerTypes<WTypes, LeafPath>>;

    template <typename WTypes, typename LeafPath>
    using GTBWWalker = memoria::bt1::FindGTBackwardWalker2<memoria::bt1::WalkerTypes<WTypes, LeafPath>>;

    template <typename WTypes, typename LeafPath>
    using GEBWWalker = memoria::bt1::FindGEBackwardWalker2<memoria::bt1::WalkerTypes<WTypes, LeafPath>>;


    void findFwGT(Int index, BigInt key)
    {
    	auto& self = this->self();
    	self.template _findFw2<GTFWWalker>(index, key);
    }

    void findFwGE(Int index, BigInt key)
    {
    	auto& self = this->self();
    	self.template _findFw2<GEFWWalker>(index, key);
    }

    void findBwGT(Int index, BigInt key)
    {
    	auto& self = this->self();
    	self.template _findBw2<GTBWWalker>(index, key);
    }

    void findBwGE(Int index, BigInt key)
    {
    	auto& self = this->self();
    	self.template _findBw2<GEBWWalker>(index, key);
    }

    template <typename T, typename P>
    using FWLeafWalker = memoria::bt1::ForwardLeafWalker<T>;

    template <typename T, typename P>
    using BWLeafWalker = memoria::bt1::BackwardLeafWalker<T>;

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mapx::ItrNavName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


//FIXME: Should nextLeaf/PreveLeaf set to End/Start if move fails?
M_PARAMS
bool M_TYPE::nextLeaf()
{
    auto& self = this->self();

    auto id = self.leaf()->id();

    self.template _findFw2<FWLeafWalker>(0, 0);

    return id != self.leaf()->id();
}




M_PARAMS
bool M_TYPE::prevLeaf()
{
    auto& self = this->self();

    auto id = self.leaf()->id();

    self.template _findBw2<BWLeafWalker>(0, 0);

    return id != self.leaf()->id();
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
    return self().template _findFw2<Types::template SkipForwardWalker>(0, amount);
}

M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
    return self().template _findBw2<Types::template SkipBackwardWalker>(0, amount);
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
