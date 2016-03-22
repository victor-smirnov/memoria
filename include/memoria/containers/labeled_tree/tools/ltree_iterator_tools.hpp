
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/types/types.hpp>


namespace memoria   {
namespace louds     {


template <typename Iterator, typename Container>
class LOUDSIteratorCache: public bt::BTreeIteratorPrefixCache<Iterator, Container> {

    typedef bt::BTreeIteratorPrefixCache<Iterator, Container>     Base;

    BigInt pos_     = 0;
    BigInt rank1_   = 0;

public:

    LOUDSIteratorCache(): Base() {}

    BigInt pos() const {
        return pos_;
    }

    BigInt rank1() const {
        return rank1_;
    }

    void setup(BigInt pos, BigInt rank1)
    {
        pos_    = pos;
        rank1_  = rank1;
    }

    void add(BigInt pos, BigInt rank1)
    {
        pos_    += pos;
        rank1_  += rank1;
    }

    void sub(BigInt pos, BigInt rank1)
    {
        pos_    -= pos;
        rank1_  -= rank1;
    }

    void setRank1(BigInt rank1)
    {
        rank1_ = rank1;
    }
};



}
}
