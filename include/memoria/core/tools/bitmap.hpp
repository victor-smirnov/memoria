
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



/**
 * Basic bitmap tools. get/set bit and bit groups. Copy/Shift bits in a Buffer.
 *
 * Buffer must have Long type declarator and [] overloaded operator.
 */

#ifndef _MEMORIA_CORE_TOOLS_BITMAP_H
#define _MEMORIA_CORE_TOOLS_BITMAP_H

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <iostream>
#include <limits>
#include <string.h>

namespace memoria    {

using namespace std;

/*
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
*/
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
inline size_t TypeBitsize() {
    return sizeof(T) * 8;
}

/**
 * Return the mask that is used to get bit address in the array cell using & operator.
 * See setBit() for example.
 */

template <typename T>
inline size_t TypeBitmask() {
    return TypeBitsize<T>() - 1;
}

/**
 * Count the bit number of a type bitmask. It is used to get cell part of bit address
 * using >> operator.
 * See setBit() for example.
 */

template <typename T>
inline size_t TypeBitmaskPopCount(T mask) {
    return mask == 7 ? 3 : (mask == 15 ? 4 : (mask == 31 ? 5 : (mask == 63 ? 6 : (mask == 127 ? 7 : 7))));
}
//
//template <typename T>
//T SimpleLg2(T value) {
//    T c, mask;
//    for (c = 0, mask = 1; mask < value; mask <<= 1, c++) {}
//    return c;
//}
//
//template <typename T>
//T SimpleSup2(BigInt value) {
//    T c, mask;
//    for (c = 1, mask = 1; mask < value; mask = (mask << 1) + 1, c++) {}
//    return c;
//}
//
//







//
//template <typename ItemType>
//ItemType CShr(ItemType value, Int count) {
//    Int bitCount = sizeof(ItemType) * 8 - count;
//    ItemType tmp = value >> (bitCount) & MakeMask<ItemType>(count, 0);
//    return (value << count) | tmp;
//}
//



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
 *              			|  size|  shift|
 * Make bitmask of type 00001111111100000000) where number of 1s is specified by
 * the size argument and mask's shift is specified by the pos argument.
 *
 * Note that optimizing compiler is able to collapse this function to one value - the mask.
 */

template <typename uT, typename sT>
uT MakeMask0(Int start, Int length)
{
    sT svalue 	= numeric_limits<sT>::min();

    Int bitsize = TypeBitsize<uT>();

    uT value 	= svalue >> (length - 1);

    return value >> (bitsize - length - start);
}

template <typename T>
struct MakeSigned: DeclType<T> {};

template <typename T>
struct MakeUnsigned: DeclType<T> {};


template <>
struct MakeSigned<UByte>: DeclType<Byte> {};

template <>
struct MakeSigned<UShort>: DeclType<Short> {};

template <>
struct MakeSigned<UInt>: DeclType<Int> {};

template <>
struct MakeSigned<UBigInt>: DeclType<BigInt> {};


template <>
struct MakeUnsigned<Byte>: DeclType<UByte> {};

template <>
struct MakeUnsigned<Short>: DeclType<UShort> {};

template <>
struct MakeUnsigned<Int>: DeclType<UInt> {};

template <>
struct MakeUnsigned<BigInt>: DeclType<UBigInt> {};

}

/**
 * Works like __make_mask(Int, Int) but takes only one type T and inferes
 * its signed equivalent basing on sizeof(T). T can be signed or unsigned.
 */
template <typename T>
T MakeMask(Int start, Int length)
{
	if (length > 0)
	{
		return (T)intrnl::MakeMask0<
					typename intrnl::MakeUnsigned<T>::Type,
					typename intrnl::MakeSigned<T>::Type
				>(start, length);
	}
	else {
		return 0;
	}
}


inline Int PopCnt(UBigInt arg)
{
	arg = arg - ((arg >> 1) & 0x5555555555555555uLL);
	arg = ((arg >> 2) & 0x3333333333333333uLL) + (arg & 0x3333333333333333uLL);
	arg = (arg + (arg >> 4)) & 0x0F0F0F0F0F0F0F0FuLL;
	UInt argl = static_cast<UInt>(arg + (arg >> 32));
	argl += (argl >> 16);
	return (argl + (argl >> 8)) & 0x7F;
}

inline Int PopCnt(UInt v)
{
	v -= ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	v = (v + (v >> 4)) & 0x0F0F0F0F;
	v = (v * 0x01010101) >> 24;
	return v;
}

inline Int PopCnt(UBigInt arg, Int start, Int length)
{
	UBigInt mask = MakeMask<UBigInt>(start, length);
	return PopCnt(arg & mask);
}

inline Int PopCnt(UInt arg, Int start, Int length)
{
	UInt mask = MakeMask<UInt>(start, length);
	return PopCnt(arg & mask);
}

template <typename Buffer>
void SetBit(Buffer& buf, size_t idx, Int value)
{
    typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t mask 	= TypeBitmask<T>();
    size_t divisor 	= TypeBitmaskPopCount(mask);

    size_t haddr 	= (idx & ~mask) >> divisor;
    size_t laddr 	= idx & mask;

    T& ref 			= buf[haddr];

    T bit			= ((T)0x1) << laddr;

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
Int GetBit(const Buffer &buf, size_t idx)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t mask = TypeBitmask<T>();
	size_t divisor = TypeBitmaskPopCount(mask);

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
void SetBits0(Buffer &buf, size_t idx, typename intrnl::ElementT<Buffer>::Type bits, Int nbits)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t mask = TypeBitmask<T>();
	size_t divisor = TypeBitmaskPopCount(mask);

	size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    T bitmask = MakeMask<T>(laddr, nbits);

    T& ref = buf[haddr];

    ref &= ~bitmask;
    ref |= (bits << laddr) & bitmask;
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
GetBits0(const Buffer &buf, size_t idx, Int nbits)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t mask = TypeBitmask<T>();
    size_t divisor = TypeBitmaskPopCount(mask);

    size_t haddr = (idx & ~mask) >> divisor;
    size_t laddr = idx & mask;

    T bitmask = MakeMask<T>(0, nbits);

    return (buf[haddr] >> laddr) & bitmask;
}

/**
 * set group of 'nbits' bits stored in 'bits' to the buffer at the address 'idx'
 *
 * Note that 0 <= nbits <= bitsize(Long)
 */

template <typename Buffer>
void SetBits(Buffer &buf, size_t idx, typename intrnl::ElementT<Buffer>::Type bits, Int nbits)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t mask 	= TypeBitmask<T>();
    size_t laddr 	= idx & mask;

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
        SetBits0(buf, idx + nbits0, bits >> (nbits0), nbits1);
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
GetBits(const Buffer &buf, size_t idx, Int nbits)
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

        return GetBits0(buf, idx, nbits0) | (GetBits0(buf, idx + nbits0, nbits1) << nbits0);
    }
}

/**
 * Move 'bitCount' bits from buffer 'src_array':srcBit to 'dst_array':dstBit.
 *
 * Note that src_aray and dst_array MUST be different buffers.
 *
 * Note that bitCount is not limited by Long.
 */

template <typename Buffer>
void MoveBitsFW(const Buffer &src, Buffer &dst, size_t src_idx, size_t dst_idx, size_t length)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

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

    	size_t divisor 		= TypeBitmaskPopCount(mask);
    	size_t start_cell 	= extent >> divisor;
    	size_t stop_cell 	= (dst_idx + length) >> divisor;

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




template <typename Buffer>
void MoveBitsBW(const Buffer &src, Buffer &dst, size_t src_idx, size_t dst_idx, size_t length)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

    size_t bitsize = TypeBitsize<T>();
    size_t mask = TypeBitmask<T>();

    size_t dst_to = dst_idx + length;
    size_t extent = dst_to -(dst_to & mask);

    if (extent <= dst_idx)
    {
    	SetBits(dst, dst_idx, GetBits(src, src_idx, length), length);
    }
    else
    {
    	size_t suffix = dst_to - extent;

    	src_idx	+= length - suffix;

    	SetBits(dst, extent, GetBits(src, src_idx, suffix), suffix);

    	size_t divisor 		= TypeBitmaskPopCount(mask);
    	size_t start_cell 	= extent >> divisor;
    	size_t stop_cell 	= dst_to >> divisor;

    	size_t cell;
    	for (cell = stop_cell; cell > start_cell; cell--, src_idx -= bitsize)
    	{
    		dst[cell] = GetBits(src, src_idx, bitsize);
    	}

    	size_t prefix = bitsize - (dst_idx & mask);

    	SetBits(dst, dst_idx, GetBits(src, src_idx - prefix, prefix), prefix);
    }
}


template <typename Buffer>
void MoveBits(const Buffer &src, Buffer &dst, size_t src_idx, size_t dst_idx, size_t length)
{
	if (dst_idx > src_idx)
	{
		MoveBitsBW(src, dst, src_idx, dst_idx, length);
	}
	else {
		MoveBitsFW(src, dst, src_idx, dst_idx, length);
	}
}


//
///**
// * Copy 'bitCount' bits from buffer 'src_array' 0 bit to 'dst_array' 0 bit.
// *
// * Note that src_aray and dst_array MUST be different buffers.
// *
// * Note that bitCount is not limited by Long.
// */
//template <typename Buffer1, typename Buffer2>
//void CopyBits(const Buffer1 &src_array, Buffer2 &dst_array, Int bitCount) {
//    CopyBits(src_array, dst_array, 0, 0, bitCount);
//}
//
///**
// * Move group of 'bitCount' bits from position 'srcBit' to 'dstBit' in the buffer 'array'
// *
// * Note that bitCount is not limited by Long.
// */
//
//template <typename Buffer>
//void ShiftBits(Buffer &array, Int srcBit, Int dstBit, Int bitCount) {
//    if (dstBit > srcBit) {
//        Int bitsize = TypeBitsize<Long>();
//
//        Int c;
//        for (c = bitCount; c >= bitsize; c -= bitsize) {
//        Long val = getBits(array, srcBit + c - bitsize, bitsize);
//            setBits(array, dstBit + c - bitsize, val, bitsize);
//        }
//
//        if (c != 0) {
//            Long val = getBits(array, srcBit, c);
//            setBits(array, dstBit, val, c);
//        }
//    }
//    else if (dstBit < srcBit) {
//        CopyBits(array, array, srcBit, dstBit, bitCount);
//    }
//}
//
//
//template <typename Int>
//void dumpAxis(std::ostream &os, Int width) {
//    for (Int c = 0, cnt = 0, cnt2 = 0; c < width;  c++, cnt++, cnt2++) {
//        if (cnt == 10) cnt = 0;
//        if (cnt2 == 20) {
//            os << ".";
//            cnt2 = 0;
//        }
//        os << cnt;
//    }
//    os<<std::endl;
//}
//
//template <typename Buffer>
//void dumpBitmap(std::ostream &os, Buffer &buffer, Int from, Int to) {
//    os.width(1);
//    for (Int c = from, cnt = 0; c < to; c++, cnt++) {
//        if (cnt == 20) {
//            os << ".";
//            cnt = 0;
//        }
//        os << (Int)getBit(buffer, c);
//    }
//    os << std::endl;
//}


/**
 * dump buffer's content in human readable form to an output stream.
 *
 * os       - a stream to output to.
 * buffer   - a buffer which content will be dumped.
 * from     - dump from
 * to       - dump to
 * width    - bits per row (100 bits by default).
 */
//template <typename Buffer>
//void dump(std::ostream &os, Buffer &buffer, Int from, Int to, Int width = 100) {
//    const Int prefix = 7;
//    for (Int c = from, cnt = 0; c < to; c += width, cnt++) {
//        Int to0 = c + width < to ? c + width : to;
//
//        if (cnt %5 == 0) {
//            os.width(prefix + 2);
//            os << "";
//            os.width(1);
//            dumpAxis(os, to0 - c);
//        }
//
//        os.width(prefix);
//        os.flags(std::ios::right | std::ios::fixed);
//        os<<cnt<<": ";
//        dumpBitmap(os, buffer, c, to0);
//    }
//}
//
//template <typename Buffer>
//void dump(std::ostream &os, Buffer &buffer, Int size) {
//    dump(os, buffer, (Int)0, size);
//}

namespace intrnl {

template <typename Buffer>
size_t CountFw(Buffer &buffer, size_t from, size_t to, const char *lut, bool zero)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

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



template <typename Buffer>
size_t CountBw(Buffer &buffer, size_t from, size_t to, const char *lut, bool zero)
{
    size_t cnt = 0;

    if (from > to)
    {
    	size_t remainder = (from - to) & 0x7; // lowerst 3 bits

    	for (BigInt c = from - 1; c > (BigInt)(to + remainder); c -= 8)
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

template <typename Buffer>
size_t CountFw(Buffer &buffer, size_t from, size_t to, const char *lut, bool zero)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t bitsize 	= TypeBitsize<T>();
	size_t mask 	= TypeBitmask<T>();

	size_t extent = from + bitsize - (from & mask);

	if (extent >= to)
	{
		return intrnl::CountFw(buffer, from, to, lut, zero);
	}

	size_t cnt = intrnl::CountFw(buffer, from, extent, lut, zero);

	if (cnt == extent - from)
	{
		size_t divisor 		= TypeBitmaskPopCount(mask);
		size_t start_cell 	= extent >> divisor;
		size_t stop_cell 	= to >> divisor;

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

template <typename Buffer>
size_t CountOneFw(Buffer &buffer, size_t from, size_t to)
{
    return CountFw(buffer, from, to, kPopCountFW_LUT, false);
}

template <typename Buffer>
size_t CountZeroFw(Buffer &buffer, size_t from, size_t to)
{
    return CountFw(buffer, from, to, kZeroCountFW_LUT, true);
}



template <typename Buffer>
size_t CountBw(Buffer &buffer, size_t from, size_t to, const char *lut, bool zero)
{
	typedef typename intrnl::ElementT<Buffer>::Type T;

	size_t bitsize 	= TypeBitsize<T>();
	size_t mask 	= TypeBitmask<T>();

	size_t extent = from - (from & mask);

	if (extent <= to || extent == 0)
	{
		return intrnl::CountBw(buffer, from, to, lut, zero);
	}

	size_t cnt = intrnl::CountBw(buffer, from, extent, lut, zero);

	if (cnt == from - extent)
	{
		size_t divisor 		= TypeBitmaskPopCount(mask);
		size_t start_cell 	= extent >> divisor;
		size_t stop_cell 	= to >> divisor;

		T value = zero ? 0 : static_cast<T>(-1);

		size_t cell;
		for (cell = start_cell; cell > stop_cell; cell--)
		{
			if (buffer[cell] == value)
			{
				cnt += bitsize;
			}
			else {
				break;
			}
		}

		cnt += intrnl::CountBw(buffer, cell<<divisor, to, lut, zero);

		return cnt;
	}
	else {
		return cnt;
	}
}



template <typename Buffer>
size_t CountOneBw(Buffer &buffer, size_t from, size_t to)
{
    return CountBw(buffer, from, to, kPopCountBW_LUT, false);
}

template <typename Buffer>
size_t CountZeroBw(Buffer &buffer, size_t from, size_t to)
{
    return CountBw(buffer, from, to, kZeroCountBW_LUT, true);
}


template <typename Buffer>
Int CreateUDS(Buffer& buf, Int start, const Int* ds, Int ds_size, Int node_bits)
{
    for (Int ids = 0; ids < ds_size; ids++)
    {
        for (Int i = 0; i < ds[ids]; i++, start++)
        {
            SetBit(buf, start, 1);
        }
        SetBit(buf, start++, 0);

        for (Int i = 0; i < ds[ids]; i++, start += node_bits)
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
void CopyBuffer(const T *src, T *dst, size_t size)
{
    memmove(dst, src, size * sizeof(T));
}


static inline void CopyByteBuffer(const void *src, void *dst, size_t size)
{
    memmove(dst, src, size);
}

template <typename T>
void MoveBuffer(T *src, long from, long to, long size)
{
    CopyBuffer(src + from, src + to, size);
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
