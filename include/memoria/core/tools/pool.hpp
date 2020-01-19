
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

/**
 * Basic bitmap tools. get/set bit and bit groups. Copy/Shift bits in a Buffer.
 *
 * Buffer must have Long type declarator and [] overloaded operator.
 */


#include <memoria/core/tools/buffer.hpp>


namespace memoria {

template <typename ID, typename Object, int32_t Size = 64>
class StaticPool {
    typedef StaticPool<ID, Object, Size> MyType;
    typedef ID T;

    T           ids_[Size];
    Object      objects_[Size];
    uint8_t     idxs_[Size];
    int32_t     size_;
    int32_t     Max;

public:
    StaticPool(): size_(0), Max(0)
    {
        for (int32_t c = 0; c < Size; c++)
        {
            ids_[c] = ID{};
        }
    }

    StaticPool(const MyType& other): size_(0), Max(0) {}

    MyType& operator=(const MyType& other) {
        return *this;
    }

    Object* get(const ID& id)
    {
        for (int32_t c = 0; c < Max; c++)
        {
            if (ids_[c] == id)
            {
                return &objects_[c];
            }
        }

        return NULL;
    }

    Object* allocate(const ID& id)
    {
        int32_t idx = selectFirst0Idx();
        if (idx < Size)
        {
            size_++;

            if (size_ > Max && Max < Size) Max = size_;

            ids_[idx] = id;
            objects_[idx].init();
            return &objects_[idx];
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("StaticPool is full");
        }
    }

    void release(const ID& id)
    {
        for (int32_t c = 0; c < Size; c++)
        {
            if (ids_[c] == id)
            {
                size_--;
                ids_[c] = ID{};
                return;
            }
        }

        MMA1_THROW(Exception()) << WhatCInfo("ID is not known in this StaticPool");
    }

    int32_t getMax() {
        return Max;
    }

    int32_t getUsage() {
        return Size - getCapacity();
    }

    int32_t getCapacity()
    {
        int32_t cnt = 0;
        for (int32_t c = 0; c < Size; c++)
        {
            if (ids_[c] == ID{})
            {
                cnt++;
            }
        }

        return cnt;
    }

    void clear() {
        for (int32_t c = 0; c < Size; c++)
        {
            ids_[c] = ID{};
        }
    }

private:
    int32_t selectFirst0Idx()
    {
        const ID EMPTY{};
        for (int32_t c = 0; c < Size; c++)
        {
            if (ids_[c] == EMPTY)
            {
                return c;
            }
        }

        return Size;
    }
};


}
