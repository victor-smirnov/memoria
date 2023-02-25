
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

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/result.hpp>


#include <memory>

namespace memoria {


template <typename Fn>
static size_t FindTotalElementsNumber2(size_t block_size, Fn&& fn)
{
    size_t first       = 0;
    size_t last        = fn.max_elements(block_size);

    size_t max_size    = 0;

    while (first < last - 1)
    {
        size_t middle = (first + last) / 2;

        auto size = fn.block_size(middle);

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

            for (size_t c = 0; c < 256; c++)
            {
                auto mid_size = fn.block_size(middle + c);
                if (mid_size <= block_size)
                {
                    max_size = middle + c;
                }
                else {
                    return max_size;
                }
            }

            MEMORIA_MAKE_GENERIC_ERROR("Can't find max_size in 64 steps. Stop.").do_throw();
        }
    }


    max_size = first;

    for (size_t c = 0; c < 1024; c++)
    {
        auto bs = fn.block_size(first + c);
        if (bs <= block_size)
        {
            max_size = first + c;
        }
        else {
            return max_size;
        }
    }

    MEMORIA_MAKE_GENERIC_ERROR("Can't find max_size in 64 steps. Stop.").do_throw();
}



template <typename Fn>
static size_t FindTotalElementsNumber(size_t first, size_t last, size_t block_size, size_t max_hops, Fn&& fn)
{
    while (first < last - 1 && max_hops > 0)
    {
        size_t middle = (first + last) / 2;

        auto size = fn(middle);

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





template <typename Tree, size_t Size>
class MultiValueSetter {

    typedef typename Tree::Value    Value;
    typedef typename Tree::Codec    Codec;

    Value values_[Size];
    size_t pos_[Size];

    Tree* tree_;

    typename Codec::BufferType* data_;

    size_t total_size_;

    Codec codec_;

public:
    MultiValueSetter(Tree* tree): tree_(tree), data_(tree->values()), total_size_(0)
    {
        for (auto& v: values_)  v = 0;
        for (auto& v: pos_)     v = 0;
    }

    size_t total_size() const {
        return total_size_;
    }

    void clearValues()
    {
        for (auto& v: values_) v = 0;
    }

    Value& value(size_t idx) {
        return values_[idx];
    }

    const Value& value(size_t idx) const {
        return values_[idx];
    }

    void putValues()
    {
        for (size_t c = 0; c < Size; c++)
        {
            size_t len = insert(pos_[c], values_[c]);
            total_size_ += len;

            for (size_t d = c; d < Size; d++)
            {
                pos_[d] += len;
            }
        }

        tree_->size() += Size;
    }

private:
    size_t insert(size_t pos, Value value)
    {
        size_t len = codec_.length(value);

        codec_.move(data_, pos, pos + len, total_size_ - pos);

        return codec_.encode(data_, value, pos, tree_->data_size());
    }
};


namespace detail {
    template <typename Fn, typename Next>
    class MultiValueFNHelper {
        Fn fn_;
        Next next_;

    public:
        MultiValueFNHelper(Fn fn, Next next) : fn_(fn), next_(next) {}

        template <typename V>
        auto operator()(size_t block, V&& value) {
            return fn_(block, std::forward<V>(value));
        }

        void next()  {
            next_();
        }
    };

    template <typename Fn>
    class MultiValueFNHelper1 {
        Fn fn_;

    public:
        MultiValueFNHelper1(Fn fn) : fn_(fn){}

        template <typename V>
        auto operator()(size_t block, V&& value) {
            return fn_(block, std::forward<V>(value));
        }

        void next()  {}
    };
}

template <typename Fn, typename NextFn>
auto make_fn_with_next(Fn&& fn, NextFn&& next)  {
    return detail::MultiValueFNHelper<Fn, NextFn>(fn, next);
}

template <typename Fn>
auto make_fn_with_next(Fn&& fn)
{
    return detail::MultiValueFNHelper1<Fn>(fn);
}

}
