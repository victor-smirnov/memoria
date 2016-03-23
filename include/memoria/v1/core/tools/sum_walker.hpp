
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace memoria {
namespace v1 {

template <typename Container, typename Key = typename Container::KeyType, bool Forward = true>
class SumTreeWalker {
public:
    Key sum_;
    Key target_;
    Int key_num_;

    Container& me_;

public:
    SumTreeWalker(Key target, Container& me, Int key_num = 0):sum_(0), target_(target), key_num_(key_num), me_(me) {}

    Key remainder() const
    {
        return target_ - sum_;
    }

    Key sum() const
    {
        return sum_;
    }

    template <typename Node>
    Int operator()(Node *node, Int idx)
    {
        if (Forward)
        {
            Int position = node->map().findFwLT(key_num_, idx, target_ - sum_, sum_);
            if (position < node->children_count())
            {
                return position;
            }
            else {
                return -1;
            }
        }
        else
        {
            return node->map().findBwLE(key_num_, idx, target_ - sum_, sum_);
        }
    }
};



}}