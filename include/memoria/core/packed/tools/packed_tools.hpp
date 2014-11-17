
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_PACKED2_TOOLS_HPP_
#define MEMORIA_CORE_PACKED2_TOOLS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {

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

            for (Int c = 0; c < 128; c++)
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

    for (Int c = 0; c < 64; c++)
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


template <typename RtnType>
struct RtnPkdHandlerBase {
    using ReturnType = RtnType;
//    using ResultType = RtnType;
};


}


#endif
