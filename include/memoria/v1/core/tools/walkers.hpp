
// Copyright 2011 Victor Smirnov
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

// FIXME: is not in use? remove

namespace memoria {
namespace v1 {

template <typename Container, typename Key = typename Container::KeyType, bool Forward = true>
class NodeTreeWalker {
public:
    Key sum_;
    Key target_;
    int32_t key_num_;

    Container& me_;

public:
    NodeTreeWalker(Key target, Container& me, int32_t key_num = 0):sum_(0), target_(target), key_num_(key_num), me_(me) {}

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
            for (int32_t c = idx; c < node->children_count(); c++)
            {
                Key key = node->map().key(key_num_, c);
                if (key + sum_ <= target_)
                {
                    sum_ = sum_ + key;
                }
                else {
                    return c;
                }
            }
        }
        else
        {
            for (int32_t c = idx; c >= 0; c--)
            {
                Key key = node->map().key(key_num_, c);
                if (key + sum_ < target_)
                {
                    sum_ = sum_ + key;
                }
                else {
                    return c;
                }
            }
        }
        return -1;
    }
};





template <typename Container, bool Forward = true, typename CountType = int64_t>
class KeyCounterWalker {

    typedef typename Container::ID          ID;
    typedef typename Container::NodeBase    NodeBase;
    typedef typename Container::NodeBaseG   NodeBaseG;

public:
    CountType sum_;
    CountType target_;

    Container& me_;

public:
    KeyCounterWalker(CountType target, Container& me):sum_(0), target_(target), me_(me) {}

    CountType remainder() const
    {
        return target_ - sum_;
    }

    CountType sum() const
    {
        return sum_;
    }

    template <typename Node>
    int32_t operator()(Node *node, int32_t idx)
    {
        if (Forward)
        {
            if (node->is_leaf())
            {
                for (int32_t c = idx; c < node->children_count(); c++)
                {
                    CountType count = 1;
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                    }
                    else {
                        return c;
                    }
                }
            }
            else {
                for (int32_t c = idx; c < node->children_count(); c++)
                {
                    NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

                    CountType count = 1;//child->counters().key_count();
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                    }
                    else {
                        return c;
                    }
                }
            }
        }
        else
        {
            if (node->is_leaf())
            {
                for (int32_t c = idx; c >= 0; c--)
                {
                    CountType count = 1;
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                    }
                    else {
                        return c;
                    }
                }
            }
            else {
                for (int32_t c = idx; c >= 0; c--)
                {
                    NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

                    CountType count = 1;//child->counters().key_count();
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                    }
                    else {
                        return c;
                    }
                }
            }
        }
        return -1;
    }
};





template <typename Container, bool Forward = true, typename CountType = int64_t, int32_t KEYS = 1>
class KeyCounterWithSumWalker {

    typedef typename Container::ID          ID;
    typedef typename Container::NodeBase    NodeBase;
    typedef typename Container::NodeBaseG   NodeBaseG;

public:
    CountType sum_;
    CountType target_;

    CountType keys_[KEYS];

    Container& me_;



public:
    KeyCounterWithSumWalker(CountType target, Container& me):sum_(0), target_(target), me_(me)
    {
        for (CountType& key: keys_) key = 0;
    }

    CountType remainder() const
    {
        return target_ - sum_;
    }

    CountType sum() const
    {
        return sum_;
    }

    CountType keys(int32_t idx) const
    {
        return keys_[idx];
    }

    template <typename Node>
    int32_t operator()(Node *node, int32_t idx)
    {
        if (Forward)
        {
            if (node->is_leaf())
            {
                for (int32_t c = idx; c < node->children_count(); c++)
                {
                    CountType count = 1;
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                        for (int32_t d = 0; d < KEYS; d++)
                        {
                            keys_[d] += node->map().key(d, c);
                        }
                    }
                    else {
                        return c;
                    }
                }
            }
            else {
                for (int32_t c = idx; c < node->children_count(); c++)
                {
                    NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

                    CountType count = 1;//child->counters().key_count();
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                        for (int32_t d = 0; d < KEYS; d++)
                        {
                            keys_[d] += node->map().key(d, c);
                        }
                    }
                    else {
                        return c;
                    }
                }
            }
        }
        else
        {
            if (node->is_leaf())
            {
                for (int32_t c = idx; c >= 0; c--)
                {
                    CountType count = 1;
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                        for (int32_t d = 0; d < KEYS; d++)
                        {
                            keys_[d] += node->map().key(d, c);
                        }
                    }
                    else {
                        return c;
                    }
                }
            }
            else {
                for (int32_t c = idx; c >= 0; c--)
                {
                    NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

                    CountType count = 1;//child->counters().key_count();
                    if (count + sum_ <= target_)
                    {
                        sum_ = sum_ + count;
                        for (int32_t d = 0; d < KEYS; d++)
                        {
                            keys_[d] += node->map().key(d, c);
                        }
                    }
                    else {
                        return c;
                    }
                }
            }
        }
        return -1;
    }
};









}}
