
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_BUFFER_H
#define _MEMORIA_CORE_TOOLS_BUFFER_H


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/typehash.hpp>

#include <iostream>



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

    StaticBuffer() {}

    const Me& operator=(const Me& other) {
        CopyBuffer(other.buffer_, buffer_, Size);
        return *this;
    }

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
    typedef ValueBuffer<Object>                                                 MyType;
    Object value_;

public:
    typedef Object                                                              ValueType;

    ValueBuffer() {}

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



#endif
