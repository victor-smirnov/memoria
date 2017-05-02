
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

#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

using namespace std;

inline uint64_t BroadwordGTZ8 (uint64_t x)
{
    uint64_t H8 = 0x8080808080808080;
    uint64_t L8 = 0x0101010101010101;

    return  ((x | (( x | H8) - L8)) & H8) >> 7;
}

inline uint64_t BroadwordLE8 (uint64_t x, uint64_t y)
{
    uint64_t H8 = 0x8080808080808080;
    return ((((y | H8) - (x & ~H8)) ^ x ^ y) & H8 ) >> 7;
}

inline size_t SelectFW(uint64_t arg, size_t rank)
{
    uint64_t v = arg;

    v -= ((v >> 1) & 0x5555555555555555);
    v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F;

    uint32_t argl = static_cast<uint32_t>(v + (v >> 32));
    argl += (argl >> 16);
    size_t fullrank_ = (argl + (argl >> 8)) & 0x7F;

    if (fullrank_ >= rank)
    {
        size_t r = 0;

        for (uint64_t shift  = 0; shift < 64; shift += 8)
        {
            uint64_t popc =  (v >> shift) & 0xFF;

            if (r + popc >= rank)
            {
                uint64_t mask = 1ull << shift;

                for (size_t d = 0; d < 8; d++, mask <<= 1)
                {
                    r += ((arg & mask) != 0);
                    if (r == rank)
                    {
                        return shift + d;
                    }
                }
            }
            else {
                r += popc;
            }
        }

        return 100 + fullrank_;
    }
    else {
        return 100 + fullrank_;
    }
}


inline size_t SelectBW(uint64_t arg, size_t rank)
{
    uint64_t v = arg;

    v -= ((v >> 1) & 0x5555555555555555);
    v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F;

    uint32_t argl = static_cast<uint32_t>(v + (v >> 32));
    argl += (argl >> 16);
    size_t fullrank_ = (argl + (argl >> 8)) & 0x7F;

    if (fullrank_ >= rank)
    {
        size_t r = 0;

        for (int32_t shift = 56; shift >= 0; shift -= 8)
        {
            uint64_t popc =  (v >> shift) & 0xFF;

            if (r + popc >= rank)
            {
                uint64_t mask = 1ull << (shift + 7);

                for (size_t d = 0; d < 8; d++, mask >>= 1)
                {
                    r += ((arg & mask) != 0);

                    if (r == rank)
                    {
                        return shift + 7 - d;
                    }
                }
            }
            else {
                r += popc;
            }
        }

        return 0;
    }
    else {
        return 100 + fullrank_;
    }
}





namespace intrnl2 {


template <typename T>
bool SelectFW(T arg, size_t& total, size_t count, size_t& stop)
{
    size_t popcnt = PopCnt(arg & MakeMask<T>(0, stop));

    if (total + popcnt < count)
    {
        total += popcnt;
        return false;
    }
    else
    {
        T mask0 = 1;

        for (size_t c = 0; c < stop; mask0 <<= 1, c++)
        {
            total += ((arg & mask0) != 0);

            if (total == count)
            {
                stop = c + 1;
                return true;
            }
        }

        return false;
    }
}


template <typename T>
bool SelectBW(T arg, size_t& total, size_t count, size_t start, size_t& delta)
{
    size_t popcnt = PopCnt(arg & MakeMask<T>(0, start + 1));

    if (total + popcnt < count)
    {
        total += popcnt;
        return false;
    }
    else
    {
        T mask0 = static_cast<T>(1) << start;

        for (size_t c = start; c > 0; mask0 >>= 1, c--)
        {
            total += ((arg & mask0) != 0);

            if (total == count)
            {
                delta = start - c;
                return true;
            }
        }

        total += ((arg & mask0) != 0);

        if (total == count)
        {
            delta = start;
            return true;
        }

        return false;
    }
}


}

class SelectResult {
    size_t  idx_;
    size_t  rank_;
    bool    found_;
public:
    SelectResult(size_t idx, size_t rank, bool found): idx_(idx), rank_(rank), found_(found) {}

    size_t idx() const   {return idx_;}
    size_t rank() const  {return rank_;}

    size_t& rank() {return rank_;}

    bool is_found() const {return found_;}
};


template <typename T>
SelectResult Select1FW(const T* buffer, size_t start, size_t stop, size_t rank)
{
    const size_t bitsize    = TypeBitsize<T>();
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t prefix   = bitsize - (start & mask);

    size_t suffix;
    size_t start_cell;
    size_t stop_cell;

    if (start + prefix >= stop)
    {
        prefix      = stop - start;
        suffix      = 0;
        start_cell  = 0;
        stop_cell   = 0;
    }
    else
    {
        suffix      = stop & mask;
        start_cell  = (start + prefix) >> divisor;
        stop_cell   = stop >> divisor;
    }

    size_t total = 0;

    size_t result = SelectFW(GetBits0(buffer, start, prefix), rank - total);
    if (result < 100)
    {
        return SelectResult(start + result, rank, true);
    }
    else {
        total += result - 100;
    }


    for (size_t cell = start_cell; cell < stop_cell; cell++)
    {
        result = SelectFW(buffer[cell], rank - total);
        if (result < 100)
        {
            return SelectResult((cell << divisor) + result, rank, true);
        }
        else
        {
            total += result - 100;
        }
    }

    if (suffix > 0)
    {
        size_t start0 = stop_cell << divisor;

        result = SelectFW(GetBits0(buffer, start0, suffix), rank - total);

        if (result < 100)
        {
            return SelectResult(start0 + result, rank, true);
        }
        else {
            return SelectResult(stop, total + result - 100, false);
        }
    }
    else {
        return SelectResult(stop, total, false);
    }
}


template <typename T>
SelectResult Select0FW(const T* buffer, size_t start, size_t stop, size_t rank)
{
    const size_t bitsize    = TypeBitsize<T>();
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t prefix   = bitsize - (start & mask);

    size_t suffix;
    size_t start_cell;
    size_t stop_cell;

    if (start + prefix >= stop)
    {
        prefix      = stop - start;
        suffix      = 0;
        start_cell  = 0;
        stop_cell   = 0;
    }
    else
    {
        suffix      = stop & mask;
        start_cell  = (start + prefix) >> divisor;
        stop_cell   = stop >> divisor;
    }

    size_t total = 0;

    size_t result = SelectFW(GetBitsNeg0(buffer, start, prefix), rank - total);
    if (result < 100)
    {
        return SelectResult(start + result, rank, true);
    }
    else {
        total += result - 100;
    }


    for (size_t cell = start_cell; cell < stop_cell; cell++)
    {
        result = SelectFW(~buffer[cell], rank - total);
        if (result < 100)
        {
            return SelectResult((cell << divisor) + result, rank, true);
        }
        else
        {
            total += result - 100;
        }
    }

    if (suffix > 0)
    {
        size_t start0 = stop_cell << divisor;

        result = SelectFW(GetBitsNeg0(buffer, start0, suffix), rank - total);

        if (result < 100)
        {
            return SelectResult(start0 + result, rank, true);
        }
        else {
            return SelectResult(stop, total + result - 100, false);
        }
    }
    else {
        return SelectResult(stop, total, false);
    }
}


template <typename T>
T GetBits00(const T* buf, size_t idx, int32_t nbits)
{
    const size_t mask = TypeBitmask<T>();
    const size_t divisor = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    size_t bitsize = TypeBitsize<T>();

    T bitmask = MakeMask<T>(bitsize - nbits, nbits);

    return (buf[haddr] << (bitsize - laddr - nbits)) & bitmask;
}


template <typename T>
T GetBitsNeg00(const T* buf, size_t idx, int32_t nbits)
{
    const size_t mask = TypeBitmask<T>();
    const size_t divisor = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    size_t bitsize = TypeBitsize<T>();

    T bitmask = MakeMask<T>(bitsize - nbits, nbits);

    return (~buf[haddr] << (bitsize - laddr - nbits)) & bitmask;
}




template <typename T>
SelectResult Select1BW(const T* buffer, size_t start, size_t stop, size_t rank)
{
    if (start - stop > 0)
    {
        const size_t bitsize    = TypeBitsize<T>();
        const size_t mask       = TypeBitmask<T>();
        const size_t divisor    = TypeBitmaskPopCount(mask);

        size_t prefix   = start & mask;
        size_t padding  = stop & mask;

        size_t suffix;
        size_t start_cell;
        size_t stop_cell;

        if (start - prefix <= stop)
        {
            prefix      = start - stop;
            suffix      = 0;
            start_cell  = 0;
            stop_cell   = 0;
        }
        else
        {
            suffix      = bitsize - padding;
            stop_cell   = stop >> divisor;
            start_cell  = ((start - prefix) >> divisor) - 1;
        }

        size_t total = 0;

        if (prefix > 0)
        {
            size_t result = SelectBW(GetBits00(buffer, start - prefix, prefix), rank);

            if (result < 100)
            {
                return SelectResult(start + result - bitsize, rank, true);
            }
            else {
                total += result - 100;
            }
        }

        for (size_t cell = start_cell; cell > stop_cell; cell--)
        {
            size_t result = SelectBW(buffer[cell], rank - total);

            if (result < 100)
            {
                return SelectResult((cell << divisor) + result, rank, true);
            }
            else {
                total += result - 100;
            }
        }

        if (suffix > 0)
        {
            size_t result = SelectBW(GetBits00(buffer, stop, suffix), rank - total);
            if (result < 100)
            {
                return SelectResult(stop - padding + result, rank, true);
            }
            else {
                return SelectResult(stop, total + result - 100, false);
            }
        }
        else {
            return SelectResult(stop, total, false);
        }
    }
    else if (rank > 0) {
        return SelectResult(stop, 0, false);
    }
    else {
        return SelectResult(stop, 0, true);
    }
}



template <typename T>
SelectResult Select0BW(const T* buffer, size_t start, size_t stop, size_t rank)
{
    if (start - stop > 0)
    {
        const size_t bitsize    = TypeBitsize<T>();
        const size_t mask       = TypeBitmask<T>();
        const size_t divisor    = TypeBitmaskPopCount(mask);

        size_t prefix   = start & mask;
        size_t padding  = stop & mask;

        size_t suffix;
        size_t start_cell;
        size_t stop_cell;

        if (start - prefix <= stop)
        {
            prefix      = start - stop;
            suffix      = 0;
            start_cell  = 0;
            stop_cell   = 0;
        }
        else
        {
            suffix      = bitsize - padding;
            stop_cell   = stop >> divisor;
            start_cell  = ((start - prefix) >> divisor) - 1;
        }



        size_t total = 0;

        if (prefix > 0)
        {
            size_t result = SelectBW(GetBitsNeg00(buffer, start - prefix, prefix), rank);

            if (result < 100)
            {
                return SelectResult(start + result - bitsize, rank, true);
            }
            else {
                total += result - 100;
            }
        }

        for (size_t cell = start_cell; cell > stop_cell; cell--)
        {
            size_t result = SelectBW(~buffer[cell], rank - total);

            if (result < 100)
            {
                return SelectResult((cell << divisor) + result, rank, true);
            }
            else {
                total += result - 100;
            }
        }

        if (suffix > 0)
        {
            size_t result = SelectBW(GetBitsNeg00(buffer, stop, suffix), rank - total);
            if (result < 100)
            {
                return SelectResult(stop - padding + result, rank, true);
            }
            else {
                return SelectResult(stop, total + result - 100, false);
            }
        }
        else {
            return SelectResult(stop, total, false);
        }
    }
    else if (rank > 0) {
        return SelectResult(stop, 0, false);
    }
    else {
        return SelectResult(stop, 0, true);
    }
}



}}
