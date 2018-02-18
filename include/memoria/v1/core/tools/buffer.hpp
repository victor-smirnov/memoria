
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


#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <iostream>
#include <type_traits>


namespace memoria {
namespace v1 {

template <size_t Size>
class StaticBuffer {
public:
    typedef uint32_t            Element;
    typedef uint32_t            ElementType;

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

    const ElementType &operator[](int32_t idx) const
    {
        return buffer_[idx];
    }

    ElementType &operator[](int32_t idx)
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


}}
