
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

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/types.hpp>


namespace memoria {
namespace louds {


template <typename Iterator, typename Container>
class LOUDSIteratorCache: public bt::BTreeIteratorPrefixCache<Iterator, Container> {

    typedef bt::BTreeIteratorPrefixCache<Iterator, Container>     Base;

    int64_t pos_     = 0;
    int64_t rank1_   = 0;

public:

    LOUDSIteratorCache(): Base() {}

    int64_t pos() const {
        return pos_;
    }

    int64_t rank1() const {
        return rank1_;
    }

    void setup(int64_t pos, int64_t rank1)
    {
        pos_    = pos;
        rank1_  = rank1;
    }

    void add(int64_t pos, int64_t rank1)
    {
        pos_    += pos;
        rank1_  += rank1;
    }

    void sub(int64_t pos, int64_t rank1)
    {
        pos_    -= pos;
        rank1_  -= rank1;
    }

    void setRank1(int64_t rank1)
    {
        rank1_ = rank1;
    }
};



}}
