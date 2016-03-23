
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

#include <utility>

namespace memoria {
namespace v1 {
namespace louds     {

class LoudsNode {
protected:
    BigInt  node_;
    BigInt  rank1_;
    Int     value_;

public:
    LoudsNode(BigInt node, BigInt rank1, Int value = -1):
        node_(node),
        rank1_(rank1),
        value_(value)
    {}

    LoudsNode():
        node_(0),
        rank1_(0),
        value_(0)
    {}

    BigInt node() const {return node_;}
    BigInt rank1() const {return rank1_;}
    BigInt rank0() const {return node_ + 1 - rank1_;}

    Int value() const {return value_;}

    bool isLeaf() const {
        return value_ == 0;
    }

    void operator++(int) {
        node_++;
        rank1_++;
    }

    bool operator<(const LoudsNode& other) const {
        return node_ < other.node_;
    }
};

static std::ostream& operator<<(std::ostream& out, const LoudsNode& node)
{
    out<<"LoudsNode["<<node.node()<<", "<<node.rank0()<<", "<<node.rank1()<<"]";
    return out;
}


class LoudsNodeRange: public LoudsNode {
    BigInt count_;
public:
    LoudsNodeRange(BigInt node, BigInt noderank_, Int value, BigInt count):
        LoudsNode(node, noderank_, value),
        count_(count)
    {}

    LoudsNodeRange(const LoudsNode& start, BigInt count):
        LoudsNode(start),
        count_(count)
    {}

    BigInt count() const {return count_;}

    LoudsNode first() const {
        return LoudsNode(node(), rank1(), value());
    }

    LoudsNode last() const {
        return LoudsNode(node() + count_, rank1() + count_, 0);
    }
};


}
}}