
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_TYPES_HPP12
#define	_MEMORIA_VAPI_TYPES_HPP12

#include <string>
#include <sstream>
#include <cassert>
#include <stdint.h>

#include <memoria/core/tools/config.hpp>

namespace memoria    {

typedef int64_t           	BigInt;
typedef uint64_t  			UBigInt;
typedef int32_t             Int;
typedef uint32_t        	UInt;
typedef int16_t             Short;
typedef uint16_t      		UShort;
typedef int8_t              Byte;
typedef uint8_t       		UByte;

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


template <Int Value>
struct CodeValue {
    static const Int Code = Value;
};

/*
 * Container type names & profiles
 */


struct BTree {};
struct BSTree {};
struct DynVector {};
struct Composite {};

struct Superblock:  public CodeValue<0> {};
struct Root:        public CodeValue<1> {};

template <Int Indexes>
struct Map:      	public CodeValue<0x618a2f + Indexes * 256> {};

typedef Map<1> 		Map1;

template <Int Indexes>
struct Set:      	public CodeValue<0x5c421d + Indexes * 256> {};

typedef Set<1> 		Set1;
typedef Set<2> 		Set2;

struct DFUDS:       public CodeValue<5> {};
struct LOUDS:       public CodeValue<6> {};
struct VectorMap:   public CodeValue<7> {};
struct Vector:   	public CodeValue<8> {};


template <typename ChildType = void>
class SmallProfile {};

/*
 * End of container type names and profiles
 */



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

template <typename First, typename Second>
struct ValuePair {

	First 	first;
	Second	second;

	ValuePair(const First& f, const Second& s): first(f), second(s) {}
	ValuePair(const First& f): first(f) {}
};

struct NoParamCtr {};
struct IterEndMark {};

struct SerializationData {
	char* buf;
	Int total;
};

struct DeserializationData {
	const char* buf;
};

}

#endif
