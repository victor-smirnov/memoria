
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

//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<Optional<T>, Indexes>& tgt, const core::StaticVector<Optional<T>, Indexes>& src) {
//    tgt = src;
//}

//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<T, Indexes>& tgt, const core::StaticVector<Optional<T>, Indexes>& src) {
//    for (int32_t c = 0; c < Indexes; c++) {
//        tgt[c] = src[c].get();
//    }
//}

//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<T, Indexes>& tgt, const core::StaticVector<T, Indexes>& src) {
//    for (int32_t c = 0; c < Indexes; c++) {
//        tgt[c] = src[c];
//    }
//}

//template <typename T, typename TT, int32_t Indexes>
//void assign(core::StaticVector<T, Indexes>& tgt, const core::StaticVector<TT, Indexes>& src) {
//    for (int32_t c = 0; c < Indexes; c++) {
//        tgt[c] = src[c];
//    }
//}

//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<Optional<T>, Indexes>& tgt, const core::StaticVector<T, Indexes>& src)
//{
//    for (int32_t c = 0; c < Indexes; c++) {
//        tgt[c] = src[c];
//    }
//}

//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<Optional<T>, Indexes>& tgt, const T& src)
//{
//    static_assert(Indexes == 1, "");
//    tgt[0] = src;
//}


//template <typename T, int32_t Indexes>
//void assign(core::StaticVector<T, Indexes>& tgt, const T& src)
//{
//    static_assert(Indexes <= 1, "");
//    if (Indexes == 1)
//    {
//        tgt[0] = src;
//    }
//}



//template <typename Fn>
//static int32_t FindTotalElementsNumber(int32_t block_size, Fn&& fn)
//{
//    int32_t first       = 1;
//    int32_t last        = block_size;

//    while (first < last - 1)
//    {
//        int32_t middle = (first + last) / 2;

//        int32_t size = fn.getBlockSize(middle);
//        if (size < block_size)
//        {
//            first = middle;
//        }
//        else if (size > block_size)
//        {
//            last = middle;
//        }
//        else {
//            break;
//        }
//    }

//    int32_t max_size;

//    if (fn.getBlockSize(last) <= block_size)
//    {
//        max_size = last;
//    }
//    else if (fn.getBlockSize((first + last) / 2) <= block_size)
//    {
//        max_size = (first + last) / 2;
//    }
//    else {
//        max_size = first;
//    }

//    int32_t max = fn.extend(max_size);

//    if (fn.getIndexSize(max) <= fn.getIndexSize(max_size))
//    {
//        return max;
//    }

//    return max_size;
//}



template <typename Fn>
static Int32Result FindTotalElementsNumber2(int32_t block_size, Fn&& fn) noexcept
{
    int32_t first       = 0;
    int32_t last        = fn.max_elements(block_size);

    int32_t max_size    = 0;

    while (first < last - 1)
    {
        int32_t middle = (first + last) / 2;

        MEMORIA_TRY(size, fn.block_size(middle));

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

            for (int32_t c = 0; c < 256; c++)
            {
                MEMORIA_TRY(mid_size, fn.block_size(middle + c));
                if (mid_size <= block_size)
                {
                    max_size = middle + c;
                }
                else {
                    return max_size;
                }
            }

            return MEMORIA_MAKE_GENERIC_ERROR("Can't find max_size in 64 steps. Stop.");
        }
    }


    max_size = first;

    for (int32_t c = 0; c < 1024; c++)
    {
        MEMORIA_TRY(bs, fn.block_size(first + c));
        if (bs <= block_size)
        {
            max_size = first + c;
        }
        else {
            return max_size;
        }
    }

    return MEMORIA_MAKE_GENERIC_ERROR("Can't find max_size in 64 steps. Stop.");
}


//template <typename Fn>
//static int32_t FindTotalElementsNumber3(int32_t block_size, Fn&& fn)
//{
//    int32_t first       = 0;
//    int32_t last        = fn.max_elements(block_size);

//    int32_t max_size    = 0;

//    while (first < last - 1)
//    {
//        int32_t middle = (first + last) / 2;

//        int32_t size = fn.block_size(middle);

//        if (size < block_size)
//        {
//            first = middle;
//        }
//        else if (size > block_size)
//        {
//            last = middle;
//        }
//        else {

//            max_size = middle;
//            return max_size;
//        }
//    }

//    max_size = first;

//    return max_size;
//}



template <typename Fn>
static Int32Result FindTotalElementsNumber(int32_t first, int32_t last, int32_t block_size, int32_t max_hops, Fn&& fn) noexcept
{
    while (first < last - 1 && max_hops > 0)
    {
        int32_t middle = (first + last) / 2;

        MEMORIA_TRY(size, fn(middle));

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





template <typename Tree, int32_t Size>
class MultiValueSetter {

    typedef typename Tree::Value    Value;
    typedef typename Tree::Codec    Codec;

    Value values_[Size];
    int32_t pos_[Size];

    Tree* tree_;

    typename Codec::BufferType* data_;

    int32_t total_size_;

    Codec codec_;

public:
    MultiValueSetter(Tree* tree): tree_(tree), data_(tree->values()), total_size_(0)
    {
        for (auto& v: values_)  v = 0;
        for (auto& v: pos_)     v = 0;
    }

    int32_t total_size() const {
        return total_size_;
    }

    void clearValues()
    {
        for (auto& v: values_)  v = 0;
    }

    Value& value(int32_t idx) {
        return values_[idx];
    }

    const Value& value(int32_t idx) const {
        return values_[idx];
    }

    void putValues()
    {
        for (int32_t c = 0; c < Size; c++)
        {
            int32_t len = insert(pos_[c], values_[c]);
            total_size_ += len;

            for (int32_t d = c; d < Size; d++)
            {
                pos_[d] += len;
            }
        }

        tree_->size() += Size;
    }

private:
    int32_t insert(int32_t pos, Value value)
    {
        int32_t len = codec_.length(value);

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
        auto operator()(int32_t block, V&& value) {
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
        auto operator()(int32_t block, V&& value) {
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

}
