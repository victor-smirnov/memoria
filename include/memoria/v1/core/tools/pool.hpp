
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/**
 * Basic bitmap tools. get/set bit and bit groups. Copy/Shift bits in a Buffer.
 *
 * Buffer must have Long type declarator and [] overloaded operator.
 */


#include <memoria/v1/core/tools/buffer.hpp>


namespace memoria {
namespace v1 {


template <typename ID, typename Object, Int Size = 64>
class StaticPool {
    typedef StaticPool<ID, Object, Size> MyType;
    typedef ID T;

    T       ids_[Size];
    Object  objects_[Size];
    UByte   idxs_[Size];
    Int     size_;
    Int     Max;

public:
    StaticPool(): size_(0), Max(0)
    {
        for (Int c = 0; c < Size; c++)
        {
            ids_[c] = ID();
        }
    }

    StaticPool(const MyType& other): size_(0), Max(0) {}

    MyType& operator=(const MyType& other) {
        return *this;
    }

    Object* get(const ID& id)
    {
        for (Int c = 0; c < Max; c++)
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
        Int idx = selectFirst0Idx();
        if (idx < Size)
        {
            size_++;

            if (size_ > Max && Max < Size) Max = size_;

            ids_[idx] = id;
            objects_[idx].init();
            return &objects_[idx];
        }
        else {
            throw new Exception(MEMORIA_SOURCE, "StaticPool is full");
        }
    }

    void release(const ID& id)
    {
        for (Int c = 0; c < Size; c++)
        {
            if (ids_[c] == id)
            {
                size_--;
                ids_[c] = ID();
                return;
            }
        }

        throw new Exception(MEMORIA_SOURCE, "ID is not known in this StaticPool");
    }

    Int getMax() {
        return Max;
    }

    Int getUsage() {
        return Size - getCapacity();
    }

    Int getCapacity()
    {
        Int cnt = 0;
        for (Int c = 0; c < Size; c++)
        {
            if (ids_[c] == ID())
            {
                cnt++;
            }
        }

        return cnt;
    }

    void clear() {
        for (Int c = 0; c < Size; c++)
        {
            ids_[c] = ID();
        }
    }

private:
    Int selectFirst0Idx()
    {
        const ID EMPTY;
        for (Int c = 0; c < Size; c++)
        {
            if (ids_[c] == EMPTY)
            {
                return c;
            }
        }

        return Size;
    }
};


}}