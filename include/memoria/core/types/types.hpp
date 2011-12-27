
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_TYPES_HPP12
#define	_MEMORIA_VAPI_TYPES_HPP12

#include <string>
#include <sstream>
#include <cassert>

#include <memoria/core/tools/config.hpp>

namespace memoria    {


struct GlobalConstants {
    //defaults:
    static const int KIND = 0;

    //values:
    static const int CACHE_LINE_WIDTH = 64;
};

typedef long long           BigInt;
typedef unsigned long long  UBigInt;
typedef int                 Int;
typedef unsigned int        UInt;
typedef short               Short;
typedef unsigned short      UShort;
typedef char                Byte;
typedef unsigned char       UByte;

template <int size> struct PlatformLongHelper;

template <>
struct PlatformLongHelper<4> {
	typedef Int 			LongType;
	typedef UInt 			ULongType;
};

template <>
struct PlatformLongHelper<8> {
	typedef BigInt 			LongType;
	typedef UBigInt 		ULongType;
};


/**
 * Please note that Long/ULong types are not intended to be used for data page properties.
 * Use types with known size instead.
 */

typedef PlatformLongHelper<sizeof(void*)>::LongType 							Long;
typedef PlatformLongHelper<sizeof(void*)>::ULongType 							ULong;

typedef std::string                                                             String;
typedef const String&                                                      		StringRef;


// FIXME: move it into the string library
template <typename T>
String ToString(const T& value, bool hex = false)
{
	std::stringstream str;
	if (hex) {
		str<<hex;
	}
	str<<value;
	return str.str();
}


template <Int Value>
struct CodeValue {
    static const Int Code = Value;
};

template <Int Value = -1>
struct CV {
    static const Int Code = Value;
};

struct BTree {};
struct ITree {};
struct DynVector {};

struct Superblock:  public CodeValue<0> {};
struct Root:        public CodeValue<1> {};

template <typename Key, typename Value>
struct KVMap:       public CodeValue<2> {};

typedef KVMap<BigInt, BigInt> DefKVMap;

template <Int Indexes>
struct IdxMap:      public CodeValue<3 + Indexes * 256> {};

typedef IdxMap<1> IdxMap1;

template <Int Indexes>
struct IdxSet:      public CodeValue<4 + Indexes * 256> {};

typedef IdxSet<1> IdxSet1;
typedef IdxSet<2> IdxSet2;

struct DFUDS:       public CodeValue<5> {};
struct LOUDS:       public CodeValue<6> {};
struct VectorMap:   public CodeValue<7> {};
struct Vector:   	public CodeValue<8> {};



struct NullType {
    typedef NullType List;
};

struct EmptyType {};
struct IncompleteType;

template <typename Name>
struct TypeNotFound;
struct TypeIsNotDefined;

template <typename Name>
struct PrintType;

template <typename FirstType, typename SecondType>
struct Pair {
    typedef FirstType   First;
    typedef SecondType  Second;
};

template <BigInt Code>
struct TypeCode {
    static const BigInt Value = Code;
};


struct TrueValue {
    static const bool Value = true;
};

struct FalseValue {
    static const bool Value = false;
};


template <typename HeadType, typename TailType = NullType>
struct TL {
    typedef TL<HeadType, TailType>    List;

    typedef HeadType                        Head;
    typedef TailType                        Tail;
};

template <typename Type_, Type_ V>
struct ValueWrapper {
    typedef Type_               Type;
    static const Type Value     = V;
};

struct Typed {
	 virtual ~Typed() throw () {}
};

class EmptyValue {
public:
	EmptyValue() {}
	EmptyValue(const BigInt) {}
	EmptyValue(const EmptyValue& other) {}
	EmptyValue& operator=(const EmptyValue& other) {
		return *this;
	}

	template <typename T>
	operator T () {
		return 0;
	}

	operator BigInt () {
		return 0;
	}
};

}

#endif
