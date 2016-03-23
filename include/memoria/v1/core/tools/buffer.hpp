
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <iostream>
#include <type_traits>


namespace memoria    {

template <size_t Size>
class StaticBuffer {
public:
    typedef UInt            Element;
    typedef UInt            ElementType;

private:
    ElementType buffer_[Size];

    typedef StaticBuffer<Size> Me;

public:

    StaticBuffer() = default;

    Me& operator=(const Me& other) = default;

    bool operator==(const Me&other) const {
        return CompareBuffers(buffer_, other.buffer_, Size);
    }

    bool operator!=(const Me&other) const {
        return !CompareBuffers(buffer_, other.buffer_, Size);
    }

    void clear()
    {
        for (size_t c = 0; c < Size; c++)
        {
            buffer_[c] = 0;
        }
    }

    const ElementType &operator[](Int idx) const
    {
        return buffer_[idx];
    }

    ElementType &operator[](Int idx)
    {
        return buffer_[idx];
    }

    void copyFrom(const void *mem) {
        CopyBuffer(mem, buffer_, Size);
    }

    void copyTo(void *mem) const {
        CopyBuffer(buffer_, mem, Size);
    }
};


template <typename Object>
class ValueBuffer {

    static_assert(std::is_trivial<Object>::value, "Object must be a trivial type");

    typedef ValueBuffer<Object>                                                 MyType;
    Object value_;

public:
    typedef Object                                                              ValueType;

    ValueBuffer() = default;
    ValueBuffer(const MyType&) = default;

    ValueBuffer(const Object &obj) {
        value() = obj;
    }

    void clear() {
        value_ = 0;
    }

    void copyTo(void *mem) const {
        CopyByteBuffer(&value_, mem, sizeof(Object));
    }

    void copyFrom(const void *mem) {
        CopyByteBuffer(mem, &value_, sizeof(Object));
    }

    const Object &value() const {
        return value_;
    }

    Object &value() {
        return value_;
    }


    bool operator<(const MyType &other) const {
        return value() < other.value();
    }
};


} //memoria
