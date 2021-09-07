
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


#include <memoria/core/types.hpp>


#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/msvc_intrinsics.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <iostream>
#include <limits>
#include <type_traits>
#include <cstring>



namespace memoria {

const char kPopCountFW_LUT [] = {0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,\
                                 0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,\
                                 0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,\
                                 0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,\
                                 0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,\
                                 0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,8,};

const char kZeroCountFW_LUT[] = {8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,\
                                 2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,\
                                 3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,\
                                 2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,\
                                 4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,\
                                 2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,};

const char kPopCountBW_LUT [] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,\
                                 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,\
                                 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,\
                                 2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,7,8,};

const char kZeroCountBW_LUT[] = {8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,\
                                 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,\
                                 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,\
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};





/**
 * Return size in bits of type T
 *
 */

template <typename T>
constexpr inline size_t TypeBitsize() {
    return sizeof(T) * 8;
}

/**
 * Return the mask that is used to get bit address in the array cell using & operator.
 * See setBit() for example.
 */

template <typename T>
constexpr inline size_t TypeBitmask() {
    return TypeBitsize<T>() - 1;
}

/**
 * Count the bit number of a type bitmask. It is used to get cell part of bit address
 * using >> operator.
 * See setBit() for example.
 */

template <typename T>
constexpr inline size_t TypeBitmaskPopCount(T mask) {
    return mask == 7 ? 3 : (mask == 15 ? 4 : (mask == 31 ? 5 : (mask == 63 ? 6 : (mask == 127 ? 7 : 7))));
}




/**
 * set one bit (0, 1) 'bit' in buffer 'buf' at address 'idx'
 *
 */

namespace intrnl {


    template <typename T>
    struct ElementT {
        typedef typename T::ElementType Type;
    };

    template <typename T>
    struct ElementT<T*> {
        typedef T Type;
    };

    template <typename T, size_t Size>
    struct ElementT<T[Size]> {
        typedef T Type;
    };


    /**
     *                          |  size|  shift|
     * Make bitmask of type 00001111111100000000) where number of 1s is specified by
     * the size argument and mask's shift is specified by the pos argument.
     *
     * Note that optimizing compiler is able to collapse this function to one value - the mask.
     */

    template <typename uT, typename sT>
    constexpr uT MakeMask0(int32_t start, int32_t length) noexcept
    {
        sT svalue   = std::numeric_limits<sT>::min();

        int32_t bitsize = TypeBitsize<uT>();

        uT value    = svalue >> (length - 1);

        return value >> (bitsize - length - start);
    }
}

/**
 * Works like __make_mask(int32_t, int32_t) but takes only one type T and inferes
 * its signed equivalent basing on sizeof(T). T can be signed or unsigned.
 */
template <typename T>
constexpr T MakeMask(int32_t start, int32_t length) noexcept
{
    if (length > 0)
    {
        return (T)intrnl::MakeMask0<
                    typename std::make_unsigned<T>::type,
                    typename std::make_signed<T>::type
                >(start, length);
    }
    else {
        return 0;
    }
}

constexpr uint64_t NumberOfBits(uint64_t value) noexcept
{
    return value < 2 ? value : 1 + NumberOfBits(value >> 1);
}


inline uint64_t ReverseBits(uint64_t value) noexcept
{
    value = (((value & 0xAAAAAAAAAAAAAAAAuLL) >> 1) | ((value & 0x5555555555555555uLL) << 1));
    value = (((value & 0xCCCCCCCCCCCCCCCCuLL) >> 2) | ((value & 0x3333333333333333uLL) << 2));
    value = (((value & 0xF0F0F0F0F0F0F0F0uLL) >> 4) | ((value & 0x0F0F0F0F0F0F0F0FuLL) << 4));
    value = (((value & 0xFF00FF00FF00FF00uLL) >> 8) | ((value & 0x00FF00FF00FF00FFuLL) << 8));
    value = (((value & 0xFFFF0000FFFF0000uLL) >> 16)| ((value & 0x0000FFFF0000FFFFuLL) << 16));

    return (value >> 32) | (value << 32);
}

inline uint32_t ReverseBits(uint32_t value) noexcept
{
    value = (((value & 0xaaaaaaaa) >> 1) | ((value & 0x55555555) << 1));
    value = (((value & 0xcccccccc) >> 2) | ((value & 0x33333333) << 2));
    value = (((value & 0xf0f0f0f0) >> 4) | ((value & 0x0f0f0f0f) << 4));
    value = (((value & 0xff00ff00) >> 8) | ((value & 0x00ff00ff) << 8));

    return (value >> 16) | (value << 16);
}

inline int32_t PopCnt(uint64_t arg) noexcept
{
    arg = arg - ((arg >> 1) & 0x5555555555555555uLL);
    arg = ((arg >> 2) & 0x3333333333333333uLL) + (arg & 0x3333333333333333uLL);
    arg = (arg + (arg >> 4)) & 0x0F0F0F0F0F0F0F0FuLL;
    uint32_t argl = static_cast<uint32_t>(arg + (arg >> 32));
    argl += (argl >> 16);
    return (argl + (argl >> 8)) & 0x7F;
}

inline int32_t PopCnt(uint32_t v) noexcept
{
    v -= ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0F0F0F0F;
    v = (v * 0x01010101) >> 24;
    return v;
}

template <typename T>
static constexpr T divUp(T value, T divider) noexcept {
    return (value / divider) + (value % divider ? 1 : 0);
}


inline int32_t PopCnt(uint8_t v) noexcept
{
    v -= ((v >> 1) & 0x55);
    v = (v & 0x33) + ((v >> 2) & 0x33);
    v = (v + (v >> 4)) & 0x0F;
    return v;
}

inline int32_t PopCnt(uint64_t arg, int32_t start, int32_t length) noexcept
{
    uint64_t mask = MakeMask<uint64_t>(start, length);
    return PopCnt(arg & mask);
}

inline int32_t PopCnt(uint32_t arg, int32_t start, int32_t length) noexcept
{
    uint32_t mask = MakeMask<uint32_t>(start, length);
    return PopCnt(arg & mask);
}

template <typename T>
inline size_t PopCount(const T* buffer, size_t start, size_t stop) noexcept
{
    const size_t bitsize    = TypeBitsize<T>();
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t prefix   = bitsize - (start & mask);

    size_t total    = 0;

    if (start + prefix > stop)
    {
        return PopCnt(buffer[start >> divisor], start & mask, stop - start);
    }
    else {
        total += PopCnt(buffer[start >> divisor], start & mask, prefix);

        for (size_t c = (start >> divisor) + 1; c < (stop >> divisor); c++)
        {
            total += PopCnt(buffer[c]);
        }

        size_t suffix = stop & mask;

        if (suffix > 0)
        {
            total += PopCnt(buffer[stop >> divisor], 0, suffix);
        }

        return total;
    }
}


template <typename T>
void FillOne(T* buffer, size_t start, size_t stop) noexcept
{
    const size_t bitsize    = TypeBitsize<T>();
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t prefix   = bitsize - (start & mask);

    if (start + prefix > stop)
    {
        T value = MakeMask<T>(start & mask, stop - start);

        buffer[start >> divisor] |= value;
    }
    else {
        buffer[start >> divisor] |= MakeMask<T>(start & mask, prefix);

        const T ALL_ONES = MakeMask<T>(0, bitsize);

        for (size_t c = (start >> divisor) + 1; c < (stop >> divisor); c++)
        {
            buffer[c] = ALL_ONES;
        }

        size_t suffix = stop & mask;

        if (suffix > 0)
        {
            buffer[stop >> divisor] |= MakeMask<T>(0, suffix);
        }
    }
}


template <typename T>
void FillZero(T* buffer, size_t start, size_t stop) noexcept
{
    const size_t bitsize    = TypeBitsize<T>();
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t prefix   = bitsize - (start & mask);

    if (start + prefix > stop)
    {
        T value = ~MakeMask<T>(start & mask, stop - start);

        buffer[start >> divisor] &= value;
    }
    else {
        buffer[start >> divisor] &= ~MakeMask<T>(start & mask, prefix);

        for (size_t c = (start >> divisor) + 1; c < (stop >> divisor); c++)
        {
            buffer[c] = 0;
        }

        size_t suffix = stop & mask;

        if (suffix > 0)
        {
            buffer[stop >> divisor] &= ~MakeMask<T>(0, suffix);
        }
    }
}




template <typename Buffer>
void SetBit(Buffer& buf, size_t idx, int32_t value) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t haddr    = (idx & ~mask) >> divisor;
    size_t laddr    = idx & mask;

    T& ref          = buf[haddr];

    T bit           = ((T)0x1) << laddr;

    if (value)
    {
        ref |= bit;
    }
    else {
        ref &= ~bit; // clear bit
    }
}

/**
 * get one bit (0, 1) from buffer 'buf' at address 'idx'.
 */

template <typename Buffer>
int32_t GetBit(const Buffer& buf, size_t idx) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    // FIXME: should we return buf[haddr] & bit_mask ?

    return (buf[haddr] >> laddr) & 0x1;
}

/**
 * set bit group for a case when the group does not span cell boundaries.
 * idx   - group offset in he buffer
 * bits  - bit group to set
 * nbits - number of bits to set starting from position 0 in 'bits'
 */

template <typename Buffer>
void
//__attribute__((always_inline))
SetBits0(Buffer& buf, size_t idx, typename intrnl::ElementT<Buffer>::Type bits, int32_t nbits) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    if (nbits > 0)
    {
        const size_t mask       = TypeBitmask<T>();
        const size_t divisor    = TypeBitmaskPopCount(mask);

        size_t haddr = (idx & ~mask) >> divisor;
        size_t laddr = idx & mask;

        T bitmask = MakeMask<T>(laddr, nbits);

        T& ref = buf[haddr];

        ref &= ~bitmask;
        ref |= (bits << laddr) & bitmask;
    }
}

/**
 * get bit group for a case when the group does not span cell boundaries.
 * idx   - group offset in he buffer
 * nbits - number of bits to set. 0 <= nbits <= bitsize(Long)
 *
 * The group will starts from position 0 in returned value. All top bits after nbits are 0.
 */

template <typename Buffer>
typename intrnl::ElementT<Buffer>::Type
MMA_V1_ALWAYS_INLINE
inline GetBits0(const Buffer& buf, size_t idx, int32_t nbits) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    if (nbits > 0)
    {
        const size_t mask       = TypeBitmask<T>();
        const size_t divisor    = TypeBitmaskPopCount(mask);

        size_t haddr = (idx & ~mask) >> divisor; // FIXME: Why negation of with mask is here?
        size_t laddr = idx & mask;

        T bitmask = MakeMask<T>(0, nbits);

        return (buf[haddr] >> laddr) & bitmask;
    }
    else {
        return 0;
    }
}

template <typename T>
inline bool TestBits(const T* buf, size_t idx, T bits, int32_t nbits) noexcept
{
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    T bitmask = MakeMask<T>(laddr, nbits);

    return (buf[haddr] & bitmask) == (bits << laddr);
}


template <typename T>
inline bool TestBit(const T* buf, size_t idx) noexcept
{
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    return buf[haddr] & (static_cast<T>(1)<<laddr);
}



template <typename T>
T GetBitsNeg0(const T* buf, size_t idx, int32_t nbits) noexcept
{
    const size_t mask       = TypeBitmask<T>();
    const size_t divisor    = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    T bitmask = MakeMask<T>(0, nbits);

    return ((~buf[haddr]) >> laddr) & bitmask;
}


/**
 * set group of 'nbits' bits stored in 'bits' to the buffer at the address 'idx'
 *
 * Note that 0 <= nbits <= bitsize(Long)
 */

template <typename Buffer>
void SetBits(Buffer& buf, size_t idx, typename intrnl::ElementT<Buffer>::Type bits, int32_t nbits) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    size_t mask     = TypeBitmask<T>();
    size_t laddr    = idx & mask;

    size_t bitsize = TypeBitsize<T>();

    if (laddr + nbits <= bitsize)
    {
        SetBits0(buf, idx, bits, nbits);
    }
    else
    {
        size_t nbits1 = laddr + nbits - bitsize;
        size_t nbits0 = nbits - nbits1;

        SetBits0(buf, idx, bits, nbits0);

        if (nbits1 > 0)
        {
            SetBits0(buf, idx + nbits0, bits >> (nbits0), nbits1);
        }
    }
}

/**
 * get the group of 'nbits' bits stored in buffer 'buf' at the address 'idx'. All top bits
 * after 'nbits' in the returned value are 0.
 *
 * Note that 0 <= nbits <= bitsize(Long)
 */

template <typename Buffer>
typename intrnl::ElementT<Buffer>::Type
MMA_V1_ALWAYS_INLINE inline
GetBits(const Buffer& buf, size_t idx, int32_t nbits) noexcept
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

    size_t mask = TypeBitmask<T>();
    size_t laddr = idx & mask;

    size_t bitsize = TypeBitsize<T>();

    if (laddr + nbits <= bitsize)
    {
        return GetBits0(buf, idx, nbits);
    }
    else
    {
        size_t nbits1 = laddr + nbits - bitsize;
        size_t nbits0 = nbits - nbits1;

        return GetBits0(buf, idx, nbits0) | (nbits1 > 0 ? (GetBits0(buf, idx + nbits0, nbits1) << nbits0) : 0);
    }
}

template <typename T>
inline T GetBits2(const T* buf, size_t& pos, size_t nbits) noexcept
{
    size_t bitsize      = TypeBitsize<T>();
    size_t mask         = TypeBitmask<T>();

    T value = 0;

    size_t idx      = pos / bitsize;
    size_t prefix   = bitsize - (pos & mask);
    size_t bit_pos  = pos & mask;

    const T ONES = static_cast<T>(-1);

    if (nbits + bit_pos> bitsize)
    {
        value = buf[idx] >> bit_pos;

        size_t suffix = nbits - prefix;
        value |= (buf[idx + 1] & (ONES >> (bitsize - suffix))) << prefix;
    }
    else {
        value = (buf[idx] >> bit_pos) & (ONES >> (bitsize - nbits));
    }

    pos += nbits;

    return value;
}

template <typename T>
void MoveBitsFW(const T* src, T* dst, size_t src_idx, size_t dst_idx, size_t length) noexcept
{
    size_t bitsize = TypeBitsize<T>();
    size_t mask = TypeBitmask<T>();

    size_t extent = dst_idx + bitsize - (dst_idx & mask);

    if (extent >= dst_idx + length)
    {
        SetBits(dst, dst_idx, GetBits(src, src_idx, length), length);
    }
    else
    {
        size_t prefix = extent - dst_idx;
        SetBits(dst, dst_idx, GetBits(src, src_idx, prefix), prefix);

        src_idx += prefix;

        const size_t divisor    = TypeBitmaskPopCount(mask);
        const size_t start_cell = extent >> divisor;
        const size_t stop_cell  = (dst_idx + length) >> divisor;

        size_t cell;
        for (cell = start_cell; cell < stop_cell; cell++, src_idx += bitsize)
        {
            dst[cell] = GetBits(src, src_idx, bitsize);
        }

        size_t suffix = (dst_idx + length) & mask;

        if (suffix > 0)
        {
            SetBits(dst, dst_idx + length - suffix, GetBits(src, src_idx, suffix), suffix);
        }
    }
}




template <typename T>
void MoveBitsBW(const T* src, T* dst, size_t src_idx, size_t dst_idx, size_t length) noexcept
{
    size_t bitsize  = TypeBitsize<T>();
    size_t mask     = TypeBitmask<T>();

    size_t dst_to = dst_idx + length;
    size_t extent = dst_to -(dst_to & mask);

    if (extent <= dst_idx)
    {
        SetBits(dst, dst_idx, GetBits(src, src_idx, length), length);
    }
    else
    {
        size_t suffix = dst_to - extent;

        src_idx += length - suffix;

        SetBits(dst, extent, GetBits(src, src_idx, suffix), suffix);

        const size_t divisor    = TypeBitmaskPopCount(mask);
        const size_t start_cell = dst_idx >> divisor;
        const size_t stop_cell  = (dst_to >> divisor) - 1;

        size_t cell;
        for (cell = stop_cell; cell > start_cell; cell--)
        {
            src_idx -= bitsize;
            dst[cell] = GetBits(src, src_idx, bitsize);
        }

        size_t prefix = bitsize - (dst_idx & mask);

        SetBits(dst, dst_idx, GetBits(src, src_idx - prefix, prefix), prefix);
    }
}


template <typename T>
void MoveBits(const T* src, T* dst, size_t src_idx, size_t dst_idx, size_t length) noexcept
{
    if (dst_idx > src_idx)
    {
        MoveBitsBW(src, dst, src_idx, dst_idx, length);
    }
    else {
        MoveBitsFW(src, dst, src_idx, dst_idx, length);
    }
}



namespace intrnl {

template <typename T>
size_t CountFw(const T* buffer, size_t from, size_t to, const char *lut, bool zero) noexcept
{
    size_t cnt = 0;

    size_t remainder = (to - from) & static_cast<size_t>(0x7); // lowest 3 bits
    size_t c;

    for (c = from; c < to - remainder; c += 8)
    {
        size_t tmp = GetBits(buffer, c, 8);
        char bits = lut[tmp];
        cnt += bits;
        if (bits < 8)
        {
            return cnt;
        }
    }

    if (remainder > 0)
    {
        size_t tmp = GetBits(buffer, to - remainder, remainder);
        if (zero)
        {
            tmp |= static_cast<size_t>(0x1) << remainder;
        }
        cnt += lut[tmp];
    }

    return cnt;
}



template <typename T>
size_t CountBw(const T* buffer, size_t from, size_t to, const char *lut, bool zero)
{
    size_t cnt = 0;

    if (from > to)
    {
        size_t remainder = (from - to) & 0x7; // lowerst 3 bits

        for (int64_t c = from - 1; c > (int64_t)(to + remainder); c -= 8)
        {
            size_t tmp = GetBits(buffer, c - 7, 8);
            char bits = lut[tmp];
            cnt += bits;
            if (bits < 8)
            {
                return cnt;
            }
        }

        if (remainder > 0)
        {
            size_t tmp = GetBits(buffer, to, remainder);
            if (zero)
            {
                tmp = ((tmp << 1) | static_cast<size_t>(0x1)) << (7 - remainder);
            }
            else {
                tmp = tmp << (8 - remainder);
            }

            cnt += lut[tmp];
        }

    }

    return cnt;
}

}

template <typename T>
size_t CountFw(const T* buffer, size_t from, size_t to, const char *lut, bool zero) noexcept
{
    size_t bitsize  = TypeBitsize<T>();
    size_t mask     = TypeBitmask<T>();

    size_t extent = from + bitsize - (from & mask);

    if (extent >= to)
    {
        return intrnl::CountFw(buffer, from, to, lut, zero);
    }

    size_t cnt = intrnl::CountFw(buffer, from, extent, lut, zero);

    if (cnt == extent - from)
    {
        const size_t divisor    = TypeBitmaskPopCount(mask);
        const size_t start_cell = extent >> divisor;
        const size_t stop_cell  = to >> divisor;

        T value = zero ? 0 : static_cast<T>(-1);

        size_t cell;
        for (cell = start_cell; cell < stop_cell; cell++)
        {
            if (buffer[cell] == value)
            {
                cnt += bitsize;
            }
            else {
                break;
            }
        }

        cnt += intrnl::CountFw(buffer, cell<<divisor, to, lut, zero);

        return cnt;
    }
    else {
        return cnt;
    }
}

template <typename T>
size_t CountOneFw(const T* buffer, size_t from, size_t to) noexcept
{
    return CountFw(buffer, from, to, kPopCountFW_LUT, false);
}

template <typename T>
size_t CountZeroFw(const T* buffer, size_t from, size_t to) noexcept
{
    return CountFw(buffer, from, to, kZeroCountFW_LUT, true);
}



template <typename T>
size_t CountBw(const T* buffer, size_t from, size_t to, const char *lut, bool zero) noexcept
{
    size_t bitsize  = TypeBitsize<T>();
    size_t mask     = TypeBitmask<T>();

    size_t extent = from - (from & mask);

    if (extent <= to || extent == 0)
    {
        return intrnl::CountBw(buffer, from, to, lut, zero);
    }

    size_t cnt = intrnl::CountBw(buffer, from, extent, lut, zero);

    if (cnt == from - extent)
    {
        const size_t divisor    = TypeBitmaskPopCount(mask);
        const size_t start_cell = (extent >> divisor) - 1;
        const size_t stop_cell  = to >> divisor;

        T value = zero ? 0 : static_cast<T>(-1);

        size_t cell;
        for (cell = start_cell; cell != stop_cell - 1; cell--)
        {
            if (buffer[cell] == value)
            {
                cnt += bitsize;
            }
            else {
                return cnt + intrnl::CountBw(buffer, (cell + 1) << divisor, to, lut, zero);
            }
        }

        return cnt;
    }
    else {
        return cnt;
    }
}

namespace details {

	
    constexpr uint64_t CtLZ_(uint64_t val, uint64_t mask) noexcept {
		uint64_t bit_off = (val & mask) == 0;
		return bit_off ? 1 + ((mask > 1) ? CtLZ_(val, mask >> 1) : 0) : 0;
	}

    constexpr uint32_t CtLZ_(uint32_t val, uint32_t mask) noexcept {
		uint32_t bit_off = (val & mask) == 0;
		return bit_off ? 1 + ((mask > 1) ? CtLZ_(val, mask >> 1) : 0) : 0;
	}

}

//template <typename Val>
constexpr uint32_t CtLZ(uint32_t val) noexcept {
	return memoria::details::CtLZ_(val, 0x80000000u);
}

constexpr uint64_t CtLZ(uint64_t val) noexcept {
	return memoria::details::CtLZ_(val, 0x8000000000000000ull);
}

//constexpr int32_t Log2(uint32_t value) { //constexpr
//    return 32 - CtLZ(value);
//}

//constexpr int32_t Log2(int32_t value) {
//    return 32 - CtLZ((uint32_t)value);
//}


constexpr int32_t Log2(uint64_t value) {
    return 64 - CtLZ(value);
}

//static inline int32_t Log2(int64_t value) {
//    return 64 - CtLZ((uint64_t)value);
//}


static inline int32_t CountTrailingZeroes(uint32_t value) noexcept {
    return __builtin_ctz(value);
}

static inline int32_t CountTrailingZeroes(uint64_t value) noexcept {
    return __builtin_ctzll(value);
}





template <typename T>
MMA_V1_ALWAYS_INLINE inline size_t CountTrailingZeroes(const T* buf, size_t pos, size_t limit) noexcept
{
    size_t bitsize      = TypeBitsize<T>();
    size_t mask         = TypeBitmask<T>();

    size_t start_cell   = pos / bitsize;
    size_t stop_cell    = limit / bitsize;

    size_t length = 0;

    size_t prefix = pos & mask;

    for (size_t c = start_cell; c < stop_cell; c++)
    {
        T val = buf[c] >> prefix;

        if (val)
        {
            length += CountTrailingZeroes(val);
            return length;
        }
        else {
            length += TypeBitsize<T>() - prefix;
            prefix = 0;
        }
    }

    size_t suffix = limit & mask;

    if (suffix > 0)
    {
        T val = buf[stop_cell] >> prefix;
        if (val)
        {
            length += CountTrailingZeroes(val);
        }
        else {
            length += (suffix - prefix);
        }
    }

    return length;
}





template <typename T>
size_t CountTrailingZeroesLight(const T* buf, size_t pos, size_t limit) noexcept
{
    return CountTrailingZeroes(GetBits(buf, pos, sizeof(T)*8));
}



template <typename Buffer>
size_t CountOneBw(const Buffer* buffer, size_t from, size_t to) noexcept
{
    return CountBw(buffer, from, to, kPopCountBW_LUT, false);
}

template <typename Buffer>
size_t CountZeroBw(const Buffer* buffer, size_t from, size_t to) noexcept
{
    return CountBw(buffer, from, to, kZeroCountBW_LUT, true);
}


template <typename Buffer>
size_t CreateUDS(Buffer* buf, size_t start, const size_t* ds, size_t ds_size, size_t node_bits) noexcept
{
    for (size_t ids = 0; ids < ds_size; ids++)
    {
        for (int32_t i = 0; i < ds[ids]; i++, start++)
        {
            SetBit(buf, start, 1);
        }
        SetBit(buf, start++, 0);

        for (size_t i = 0; i < ds[ids]; i++, start += node_bits)
        {
            SetBits(buf, start, 0x9, node_bits);
        }

        if (ds[ids] > 0)
        {
            SetBit(buf, start++, 0);
        }
    }

    return start;
}


template <typename T>
void CopyBuffer(const T *src, T *dst, size_t size) noexcept
{
    static_assert(std::is_trivially_copyable<T>::value, "CopyBuffer supports only trivially copyable types");
    std::memmove(dst, src, size * sizeof(T));
}

template <typename T>
void MemCpyBuffer(const T *src, T *dst, size_t size) noexcept
{
    static_assert(std::is_trivially_copyable<T>::value, "MemCpyBuffer supports only trivially copyable types");
    std::memcpy(dst, src, size * sizeof(T));
}

template <typename T>
void MemMoveBuffer(const T *src, T *dst, size_t size) noexcept
{
    static_assert(std::is_trivially_copyable<T>::value, "MemCpyBuffer supports only trivially copyable types");
    std::memmove(dst, src, size * sizeof(T));
}



static inline void CopyByteBuffer(const void *src, void *dst, size_t size) noexcept
{
    std::memmove(dst, src, size);
}

template <typename T>
void MoveBuffer(T *src, long from, long to, long size) noexcept
{
    CopyBuffer(src + from, src + to, size);
}

}
