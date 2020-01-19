
// Copyright 2012 Victor Smirnov
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

// FIXME: is not it use? remove

namespace memoria {

template <typename Container, typename Key = typename Container::KeyType, bool Forward = true>
class SumTreeWalker {
public:
    Key sum_;
    Key target_;
    int32_t key_num_;

    Container& me_;

public:
    SumTreeWalker(Key target, Container& me, int32_t key_num = 0):sum_(0), target_(target), key_num_(key_num), me_(me) {}

    Key remainder() const
    {
        return target_ - sum_;
    }

    Key sum() const
    {
        return sum_;
    }

    template <typename Node>
    int32_t operator()(Node *node, int32_t idx)
    {
        if (Forward)
        {
            int32_t position = node->map().findFwLT(key_num_, idx, target_ - sum_, sum_);
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



}
