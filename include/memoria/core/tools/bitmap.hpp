
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



/**
 * Basic bitmap tools. Get/Set bit and bit groups. Copy/Shift bits in a Buffer.
 *
 * Buffer must have Long type declarator and [] overloaded operator.
 */

#ifndef _MEMORIA_CORE_TOOLS_BITMAP_H
#define _MEMORIA_CORE_TOOLS_BITMAP_H

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <iostream>

namespace memoria    {

using namespace std;

template <typename ItemType, Int BitCount> struct MakeSimpleMask;

template <typename ItemType>
struct MakeSimpleMask<ItemType, 1> {
    static const ItemType Value = 1;
};

template <typename ItemType>
struct MakeSimpleMask<ItemType, 0> {
    static const ItemType Value = 0;
};

template <typename ItemType, Int BitCount>
struct MakeSimpleMask {
    static const ItemType Value = MakeSimpleMask<ItemType, BitCount - 1>::Value + (1 << (BitCount - 1));
};

template <typename ItemType, Int BitCount, Int Offset>
struct MakeMaskTool {
    static const ItemType Value = MakeSimpleMask<ItemType, BitCount>::Value << Offset;
};

template <typename ItemType, ItemType Value_, Int Count>
class CSHR {
    static const ItemType Tmp = (Value_ >> (sizeof(ItemType) * 8 - Count)) & MakeSimpleMask<ItemType, Count>::Value;
public:
    static const ItemType Value = (Value_ << Count) | Tmp;
};




#define SIMPLE_LG2_32(value) \
    (value >= 0xFFFFFFFF ? 32 : (value >= 0x7FFFFFFF ? 31 : (value >= 0x3FFFFFFF ? 30 : (value >= 0x1FFFFFFF ? 29 : \
    (value >= 0xFFFFFFF  ? 28 : (value >= 0x7FFFFFF  ? 27 : (value >= 0x3FFFFFF  ? 26 : (value >= 0x1FFFFFF  ? 25 : \
    (value >= 0xFFFFFF   ? 24 : (value >= 0x7FFFFF   ? 23 : (value >= 0x3FFFFF   ? 22 : (value >= 0x1FFFFF   ? 21 : \
    (value >= 0xFFFFF    ? 20 : (value >= 0x7FFFF    ? 19 : (value >= 0x3FFFF    ? 18 : (value >= 0x1FFFF    ? 17 : \
    (value >= 0xFFFF ? 16 : (value >= 0x7FFF ? 15 : (value >= 0x3FFF ? 14 : (value >= 0x1FFF ? 13 :     \
    (value >= 0xFFF  ? 12 : (value >= 0x7FF  ? 11 : (value >= 0x3FF  ? 10 : (value >= 0x1FF  ? 9  :     \
    (value >= 0xFF   ?  8 : (value >= 0x7F   ?  7 : (value >= 0x3F   ?  6 : (value >= 0x1F   ? 5  :     \
    (value >= 0xF    ?  4 : (value >= 0x7    ?  3 : (value >= 0x3    ?  2 : (value >= 0x1    ? 1  : 0   \
    ))))))))))))))))))))))))))))))))

#define SIMPLE_LG2_16(value) SIMPLE_LG2_32(value)

const char kPopCountFW_LUT [] = {0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,8,};
const char kZeroCountFW_LUT[] = {8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,};

const char kPopCountBW_LUT [] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,7,8,};
const char kZeroCountBW_LUT[] = {8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

/**
 * Return size in bits of type T
 *
 */

template <typename T>
inline T TypeBitsize() {
    return sizeof(T) * 8;
}

/**
 * Return the mask that is used to get bit address in the array cell using & operator.
 * See set_bit() for example.
 */

template <typename T>
inline T TypeBitmask() {
    return TypeBitsize<T>() - 1;
}

/**
 * Count the bit number of a type bitmask. It is used to get cell part of bit address
 * using >> operator.
 * See set_bit() for example.
 */

template <typename T>
inline T TypeBitmaskPopCount(T mask) {
    return mask == 7 ? 3 : (mask == 15 ? 4 : (mask == 31 ? 5 : (mask == 63 ? 6 : (mask == 127 ? 7 : 7))));
}

template <typename T>
T SimpleLg2(T value) {
    T c, mask;
    for (c = 0, mask = 1; mask < value; mask <<= 1, c++) {}
    return c;
}

template <typename T>
T SimpleSup2(BigInt value) {
    T c, mask;
    for (c = 1, mask = 1; mask < value; mask = (mask << 1) + 1, c++) {}
    return c;
}


/**
 *			    |  size|  shift|
 * Make bitmask od type 00001111111100000000) where number of 1s is specified by
 * the size argument and mask's shift is specified by the pos argument.
 *
 * Note that optimizing compiler is able to collapse this function to one value - the mask.
 */

template <typename T, typename sT>
T MakeMask0(Int size, Int pos) {
    T bitsize = TypeBitsize<T>();
    sT svalue = ((sT)0x1) << (bitsize - 1);

    if (pos + size == bitsize) {
        return (T)(svalue >> (size - 1));
    }
    else {
        sT mask = ~svalue;
        return (T)((svalue >> size) & mask) >> (bitsize - size - 1 - pos);
    }
}

/**
 * Works like __make_mask(Int, Int) but takes only one type T and inferes
 * its signed equivalent basing on sizeof(T). T can be signed or unsigned.
 */
template <typename T>
T MakeMask(Int size, Int pos) {
    if (sizeof(T) == 1) {
        return MakeMask0<T, Byte>(size, pos);
    }
    else if (sizeof(T) == 2) {
        return MakeMask0<T, Short>(size, pos);
    }
    else if (sizeof(T) == 4) {
        return MakeMask0<T, Int>(size, pos);
    }
    else if (sizeof(T) == 8) {
        return MakeMask0<T, BigInt>(size, pos);
    }
    else {
        return 0;
    }
}

template <typename ItemType>
ItemType CShr(ItemType value, Int count) {
    Int bitCount = sizeof(ItemType) * 8 - count;
    ItemType tmp = value >> (bitCount) & MakeMask<ItemType>(count, 0);
    return (value << count) | tmp;
}

/**
 * Set one bit (0, 1) 'bit' in buffer 'buf' at address 'idx'
 *
 */

template <typename Buffer>
void SetBit(Buffer &buf, Int idx, Long bit) {
    Int mask = TypeBitmask<Long>();
    Int divisor = TypeBitmaskPopCount(mask);

    Int haddr = (idx & ~mask) >> divisor;
    Int laddr = idx & mask;

    const Long x1 = 0x1;

    buf[haddr] = buf[haddr] & ~(x1 << laddr);

    if ((bit & x1) == 1) {
        buf[haddr] = buf[haddr] | ((bit & x1) << laddr);
    }
}

/**
 * Get one bit (0, 1) from buffer 'buf' at address 'idx'.
 */

template <typename Buffer>
Long GetBit(const Buffer &buf, Int idx) {
    Int mask = TypeBitmask<Long>();
    Int divisor = TypeBitmaskPopCount(mask);

    Int haddr = (idx & ~mask) >> divisor;
    Int laddr = idx & mask;

    return (buf[haddr] >> laddr) & 0x1;
}

/**
 * Set bit group for a case when the group does not span cell boundaries.
 * idx   - group offset in he buffer
 * bits  - bit group to set
 * nbits - number of bits to set starting from position 0 in 'bits'
 */

template <typename Buffer>
void SetBits0(Buffer &buf, Int idx, Long bits, Int nbits) {
    Int mask = TypeBitmask<Long>();
    Int divisor = TypeBitmaskPopCount(mask);

    Int haddr = (idx & ~mask) >> divisor;
    Int laddr = idx & mask;

    Long bitmask = MakeMask<Long>(nbits, laddr);

    buf[haddr] = buf[haddr] & ~bitmask;
    buf[haddr] = buf[haddr] | ((bits << laddr) & bitmask);
}

/**
 * Get bit group for a case when the group does not span cell boundaries.
 * idx   - group offset in he buffer
 * nbits - number of bits to set. 0 <= nbits <= bitsize(Long)
 *
 * The grou will starts from position 0 in returned value. All top bits after nbits are 0.
 */

template <typename Buffer>
Long GetBits0(const Buffer &buf, Int idx, Int nbits) {
    Int mask = TypeBitmask<Long>();
    Int divisor = TypeBitmaskPopCount(mask);

    Int haddr = (idx & ~mask) >> divisor;
    Int laddr = idx & mask;

    Long bitmask = MakeMask<Long>(nbits, 0);

    return (buf[haddr] >> laddr) & bitmask;
}

/**
 * Set group of 'nbits' bits stored in 'bits' to the buffer at the address 'idx'
 *
 * Note that 0 <= nbits <= bitsize(Long)
 */

template <typename Buffer>
void SetBits(Buffer &buf, Int idx, Long bits, Int nbits) {
    Int mask = TypeBitmask<Long>();
    Int laddr = idx & mask;

    Long bitsize = TypeBitsize<Long>();

    if (laddr + nbits <= bitsize) {
        SetBits0(buf, idx, bits, nbits);
    }
    else {
        Long nbits1 = laddr + nbits - bitsize;
        Long nbits0 = nbits - nbits1;

        SetBits0(buf, idx, bits, nbits0);
        SetBits0(buf, idx + nbits0, bits >> (nbits0), nbits1);
    }
}

/**
 * Get the group of 'nbits' bits stored in buffer 'buf' at the address 'idx'. All top bits
 * after 'nbits' in the returned value are 0.
 *
 * Note that 0 <= nbits <= bitsize(Long)
 */

template <typename Buffer>
Long GetBits(const Buffer &buf, Int idx, Int nbits) {
    Int mask = TypeBitmask<Long>();
    Int laddr = idx & mask;

    Long bitsize = TypeBitsize<Long>();

    if (laddr + nbits <= bitsize) {
        return GetBits0(buf, idx, nbits);
    }
    else {
        Long nbits1 = laddr + nbits - bitsize;
        Long nbits0 = nbits - nbits1;

        return GetBits0(buf, idx, nbits0) | (GetBits0(buf, idx + nbits0, nbits1) << nbits0);
    }
}

/**
 * Copy 'bitCount' bits from buffer 'src_array':srcBit to 'dst_array':dstBit.
 *
 * Note that src_aray and dst_array MUST be different buffers.
 *
 * Note that bitCount is not limited by Long.
 */

template <typename Buffer1, typename Buffer2>
void CopyBits(const Buffer1 &src_array, Buffer2 &dst_array, Int srcBit, Int dstBit, Int bitCount) {

    Int bitsize = TypeBitsize<Long>();
    Int bitmask = TypeBitmask<Long>();

    Int c;
    for (c=0; c < (bitCount & ~bitmask); c += bitsize) {
        SetBits(dst_array, dstBit + c, GetBits(src_array, srcBit + c, bitsize), bitsize);
    }

    Int remainder = bitCount - c;

    if (remainder != 0) {
        Int base = bitCount & ~bitmask;
        Long val = GetBits(src_array, srcBit + base, remainder);
//        std::cout<<"VAL: "<<val<<" "<<srcBit + base<<" "<<(void*)src_array<<std::endl;
        SetBits(dst_array, dstBit + base, val, remainder);
    }
}

/**
 * Copy 'bitCount' bits from buffer 'src_array' 0 bit to 'dst_array' 0 bit.
 *
 * Note that src_aray and dst_array MUST be different buffers.
 *
 * Note that bitCount is not limited by Long.
 */
template <typename Buffer1, typename Buffer2>
void CopyBits(const Buffer1 &src_array, Buffer2 &dst_array, Int bitCount) {
    CopyBits(src_array, dst_array, 0, 0, bitCount);
}

/**
 * Move group of 'bitCount' bits from position 'srcBit' to 'dstBit' in the buffer 'array'
 *
 * Note that bitCount is not limited by Long.
 */

template <typename Buffer>
void ShiftBits(Buffer &array, Int srcBit, Int dstBit, Int bitCount) {
    if (dstBit > srcBit) {
        Int bitsize = TypeBitsize<Long>();

        Int c;
        for (c = bitCount; c >= bitsize; c -= bitsize) {
	    Long val = GetBits(array, srcBit + c - bitsize, bitsize);
            SetBits(array, dstBit + c - bitsize, val, bitsize);
        }

        if (c != 0) {
            Long val = GetBits(array, srcBit, c);
            SetBits(array, dstBit, val, c);
        }
    }
    else if (dstBit < srcBit) {
        CopyBits(array, array, srcBit, dstBit, bitCount);
    }
}


template <typename Int>
void DumpAxis(std::ostream &os, Int width) {
    for (Int c = 0, cnt = 0, cnt2 = 0; c < width;  c++, cnt++, cnt2++) {
        if (cnt == 10) cnt = 0;
        if (cnt2 == 20) {
            os << ".";
            cnt2 = 0;
        }
        os << cnt;
    }
    os<<std::endl;
}

template <typename Buffer>
void DumpBitmap(std::ostream &os, Buffer &buffer, Int from, Int to) {
    os.width(1);
    for (Int c = from, cnt = 0; c < to; c++, cnt++) {
        if (cnt == 20) {
            os << ".";
            cnt = 0;
        }
        os << (Int)GetBit(buffer, c);
    }
    os << std::endl;
}

/**
 * Dump buffer's content in human readable form to an output stream.
 *
 * os	    - a stream to output to.
 * buffer   - a buffer which content will be dumped.
 * from	    - dump from
 * to	    - dump to
 * width    - bits per row (100 bits by default).
 */
template <typename Buffer>
void Dump(std::ostream &os, Buffer &buffer, Int from, Int to, Int width = 100) {
    const Int prefix = 7;
    for (Int c = from, cnt = 0; c < to; c += width, cnt++) {
        Int to0 = c + width < to ? c + width : to;

        if (cnt %5 == 0) {
            os.width(prefix + 2);
            os << "";
            os.width(1);
            DumpAxis(os, to0 - c);
        }

        os.width(prefix);
        os.flags(std::ios::right | std::ios::fixed);
        os<<cnt<<": ";
        DumpBitmap(os, buffer, c, to0);
    }
}

template <typename Buffer>
void Dump(std::ostream &os, Buffer &buffer, Int size) {
    Dump(os, buffer, (Int)0, size);
}

template <typename Buffer>
Int CountFw(Buffer &buffer, Int from, Int to, const char *lut, bool zero) {
    Int cnt = 0;

    Int reminder = (to - from) & 0x7; // lowerst 3 bits
    Int c;
    for (c = from; c < to - reminder; c += 8) {
        Long tmp = GetBits(buffer, c, 8);
        char bits = lut[tmp];
        cnt += bits;
        if (bits < 8) {
            return cnt;
        }
    }

    if (reminder > 0) {
        Long tmp = GetBits(buffer, to - reminder, reminder);
        if (zero) {
            tmp |= 0x1 << reminder;
        }
        cnt += lut[tmp];
    }

    return cnt;
}

template <typename Buffer>
Int CountOneFw(Buffer &buffer, Int from, Int to) {
    return CountFw(buffer, from, to, kPopCountFW_LUT, false);
}

template <typename Buffer>
Int CountZeroFw(Buffer &buffer, Int from, Int to) {
    return CountFw(buffer, from, to, kZeroCountFW_LUT, true);
}

template <typename Buffer>
Int CountBw(Buffer &buffer, Int from, Int to, const char *lut, bool zero) {
    Int cnt = 0;

    Int reminder = (from - to) & 0x7; // lowerst 3 bits
    Int c;
    for (c = from; c > to + reminder; c -= 8) {
        Long tmp = GetBits(buffer, c - 7, 8);
        char bits = lut[tmp];
        cnt += bits;
        if (bits < 8) {
            return cnt;
        }
    }

    if (reminder > 0) {
        Long tmp = GetBits(buffer, to + 1, reminder);
        if (zero) {
            tmp = ((tmp << 1) | 0x1) << (7 - reminder);;
        }
        else {
            tmp = tmp << (8 - reminder);
        }

        cnt += lut[tmp];
    }

    return cnt;
}

template <typename Buffer>
Int CountOneBw(Buffer &buffer, Int from, Int to) {
    return CountBw(buffer, from, to, kPopCountBW_LUT, false);
}

template <typename Buffer>
Int CountZeroBw(Buffer &buffer, Int from, Int to) {
    return CountBw(buffer, from, to, kZeroCountBW_LUT, true);
}


template <typename Buffer>
Int CreateUDS(Buffer &buf, Int start, Int *ds, Int ds_size, Int node_bits) {
    for (Int ids = 0; ids < ds_size; ids++) {
        for (Int i = 0; i < ds[ids]; i++, start++) {
            SetBit(buf, start, 1);
        }
        SetBit(buf, start++, 0);

        for (Int i = 0; i < ds[ids]; i++, start += node_bits) {
            SetBits(buf, start, 0x9, node_bits);
        }

        if (ds[ids] > 0) {
            SetBit(buf, start++, 0);
        }
    }

    return start;
}

//template <typename T>
//void copy_buffer(const char *src, char *dst, long size) {
//    const T *isrc = reinterpret_cast<const T*>(src);
//    T *idst = reinterpret_cast<T*>(dst);
//    if (dst < src) {
//        for (UInt l = 0; l < size / sizeof(T); l++) {
//            idst[l] = isrc[l];
//        }
//    }
//    else {
//        for (UInt l = size / sizeof(T) - 1; l >= 0; l--) {
//            idst[l] = isrc[l];
//        }
//    }
//}

template <typename T>
void copy_buffer(const char *src, char *dst, long size)
{
    const T *isrc = CP2CP<T>(src);
    T *idst = T2T<T*>(dst);

    if (dst < src) {
        for (UInt l = 0; l < size / sizeof(T); l++) {
            idst[l] = isrc[l];
        }
    }
    else {
        for (UInt l = size / sizeof(T) - 1; l >= 0; l--) {
            idst[l] = isrc[l];
        }
    }
}



//static inline void CopyBuffer(const void *src, void *dst, long size) {
//	cout<<"CopyBuffer: "<<src<<" "<<dst<<" "<<size<<endl;
//
//    const char* csrc = (const char*) src;
//    char*       cdst = (char*) dst;
//
//    const unsigned long long isrc = (const unsigned long long) src;
//
//    long prefix = isrc % sizeof (unsigned long) == 0 ? 0 : sizeof (unsigned long) - isrc % sizeof (unsigned long);
//    long suffix = (isrc + size) % sizeof (unsigned long);
//    long body = (size - prefix - suffix);
//
//    cout<<"buff: "<<prefix<<" "<<suffix<<" "<<body<<endl;
//
//    if (prefix > 0) copy_buffer<char>(csrc, cdst, prefix);
//    if (body   > 0) copy_buffer<long>(csrc + prefix, cdst + prefix, body);
//    if (suffix > 0) copy_buffer<char>(csrc + prefix + body, cdst + prefix + body, suffix);
//}


static inline void CopyBuffer(const void *src, void *dst, long size)
{
    typedef long LongType;

	const LongType *isrc = CP2CP<LongType>(src);
	LongType *idst = T2T<LongType*>(dst);

    unsigned long l;
    for (l = 0; l < size / sizeof(LongType); l++)
    {
        idst[l] = isrc[l];
    }

    if (size % sizeof(LongType) != 0)
    {
        const char* csrc = ((const char*) src) + l * sizeof(LongType);
        char* cdst = ((char*) dst) + l * sizeof(LongType);

        for (l = 0; l < size % sizeof(LongType); l++) {
            cdst[l] = csrc[l];
        }
    }
}


static inline void MoveBuffer(void *src, long from, long to, long size)
{
    char* csrc = (char*)src;
    CopyBuffer(csrc + from, csrc + to, size);
}

static inline bool CompareBuffers(const void *src, const void *dst, long size)
{
    const long *isrc = CP2CP<long>(src);
    long *idst = T2T<long*>(dst);

    unsigned long l;
    for (l = 0; l < size / sizeof(long); l++)
    {
        if (idst[l] != isrc[l])
        {
            return false;
        }
    }

    if (size % sizeof(long) != 0)
    {
        const char* csrc = ((const char*) src) + l * sizeof(long);
        char* cdst = ((char*) dst) + l * sizeof(long);

        for (l = 0; l < size % sizeof(long); l++)
        {
            if (cdst[l] != csrc[l])
            {
                return false;
            }
        }
    }

    return true;
}


static inline void Clean(void *dst, long size)
{
    long *idst = T2T<long*>(dst);

    unsigned long l;
    for (l = 0; l < size / sizeof(long); l++)
    {
        idst[l] = 0l;
    }

    if (size % sizeof(long) != 0)
    {
        char* cdst = ((char*) dst) + l * sizeof(long);

        for (l = 0; l < size % sizeof(long); l++)
        {
            cdst[l] = 0l;
        }
    }
}

static inline bool IsClean(const void *dst, long size)
{
    const long *idst = CP2CP<long>(dst);

    unsigned long l;
    for (l = 0; l < size / sizeof(long); l++)
    {
        if (idst[l] != 0)
        {
            return false;
        }
    }

    if (size % sizeof(long) != 0)
    {
        const char* cdst = ((const char*) dst) + l * sizeof(long);

        for (l = 0; l < size % sizeof(long); l++)
        {
            if (cdst[l] != 0)
            {
                return false;
            }
        }
    }

    return true;
}

} //memoria

#endif
