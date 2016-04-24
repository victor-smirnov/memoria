
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Fn>
static Int FindTotalElementsNumber(Int block_size, Fn&& fn)
{
    Int first       = 1;
    Int last        = block_size;

    while (first < last - 1)
    {
        Int middle = (first + last) / 2;

        Int size = fn.getBlockSize(middle);
        if (size < block_size)
        {
            first = middle;
        }
        else if (size > block_size)
        {
            last = middle;
        }
        else {
            break;
        }
    }

    Int max_size;

    if (fn.getBlockSize(last) <= block_size)
    {
        max_size = last;
    }
    else if (fn.getBlockSize((first + last) / 2) <= block_size)
    {
        max_size = (first + last) / 2;
    }
    else {
        max_size = first;
    }

    Int max = fn.extend(max_size);

    if (fn.getIndexSize(max) <= fn.getIndexSize(max_size))
    {
        return max;
    }

    return max_size;
}



template <typename Fn>
static Int FindTotalElementsNumber2(Int block_size, Fn&& fn)
{
    Int first       = 0;
    Int last        = fn.max_elements(block_size);

    Int max_size    = 0;

    while (first < last - 1)
    {
        Int middle = (first + last) / 2;

        Int size = fn.block_size(middle);

        if (size < block_size)
        {
            first = middle;
        }
        else if (size > block_size)
        {
            last = middle;
        }
        else {

            max_size = middle;

            for (Int c = 0; c < 256; c++)
            {
                if (fn.block_size(middle + c) <= block_size)
                {
                    max_size = middle + c;
                }
                else {
                    return max_size;
                }
            }

            throw Exception(MA_SRC, "Can't find max_size in 64 steps. Stop.");
        }
    }


    max_size = first;

    for (Int c = 0; c < 1024; c++)
    {
        Int bs = fn.block_size(first + c);
        if (bs <= block_size)
        {
            max_size = first + c;
        }
        else {
            return max_size;
        }
    }

    throw Exception(MA_SRC, "Can't find max_size in 64 steps. Stop.");

    return max_size;
}


template <typename Fn>
static Int FindTotalElementsNumber3(Int block_size, Fn&& fn)
{
    Int first       = 0;
    Int last        = fn.max_elements(block_size);

    Int max_size    = 0;

    while (first < last - 1)
    {
        Int middle = (first + last) / 2;

        Int size = fn.block_size(middle);

        if (size < block_size)
        {
            first = middle;
        }
        else if (size > block_size)
        {
            last = middle;
        }
        else {

            max_size = middle;
            return max_size;
        }
    }

    max_size = first;

    return max_size;
}



template <typename Fn>
static Int FindTotalElementsNumber(Int first, Int last, Int block_size, Int max_hops, Fn&& fn)
{
    while (first < last - 1 && max_hops > 0)
    {
        Int middle = (first + last) / 2;

        Int size = fn(middle);

        if (size < block_size)
        {
            max_hops--;
            first = middle;
        }
        else if (size > block_size)
        {
            last = middle;
        }
        else {
            return middle;
        }
    }

    return first;
}





template <typename Tree, Int Size>
class MultiValueSetter {

    typedef typename Tree::Value    Value;
    typedef typename Tree::Codec    Codec;

    Value values_[Size];
    Int pos_[Size];

    Tree* tree_;

    typename Codec::BufferType* data_;

    Int total_size_;

    Codec codec_;

public:
    MultiValueSetter(Tree* tree): tree_(tree), data_(tree->values()), total_size_(0)
    {
        for (auto& v: values_)  v = 0;
        for (auto& v: pos_)     v = 0;
    }

    Int total_size() const {
        return total_size_;
    }

    void clearValues()
    {
        for (auto& v: values_)  v = 0;
    }

    Value& value(Int idx) {
        return values_[idx];
    }

    const Value& value(Int idx) const {
        return values_[idx];
    }

    void putValues()
    {
        for (Int c = 0; c < Size; c++)
        {
            Int len = insert(pos_[c], values_[c]);
            total_size_ += len;

            for (Int d = c; d < Size; d++)
            {
                pos_[d] += len;
            }
        }

        tree_->size() += Size;
    }

private:
    Int insert(Int pos, Value value)
    {
        Int len = codec_.length(value);

        codec_.move(data_, pos, pos + len, total_size_ - pos);

        return codec_.encode(data_, value, pos, tree_->data_size());
    }
};


namespace {
    template <typename Fn, typename Next>
    class MultiValueFNHelper {
        Fn fn_;
        Next next_;

    public:
        MultiValueFNHelper(Fn fn, Next next): fn_(fn), next_(next) {}

        template <typename V>
        auto operator()(Int block, V&& value) {
            return fn_(block, std::forward<V>(value));
        }

        void next() {
            next_();
        }
    };

    template <typename Fn>
    class MultiValueFNHelper1 {
        Fn fn_;

    public:
        MultiValueFNHelper1(Fn fn): fn_(fn){}

        template <typename V>
        auto operator()(Int block, V&& value) {
            return fn_(block, std::forward<V>(value));
        }

        void next() {}
    };
}

template <typename Fn, typename NextFn>
auto make_fn_with_next(Fn&& fn, NextFn&& next) {
    return MultiValueFNHelper<Fn, NextFn>(fn, next);
}

template <typename Fn>
auto make_fn_with_next(Fn&& fn)
{
    return MultiValueFNHelper1<Fn>(fn);
}







}}
