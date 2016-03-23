
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/tools/isequencedata.hpp>
#include <memoria/v1/core/types/types.hpp>


namespace memoria {
namespace v1 {
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
}}