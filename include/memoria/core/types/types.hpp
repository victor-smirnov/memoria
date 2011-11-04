
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_TYPES_HPP12
#define	_MEMORIA_VAPI_TYPES_HPP12

#include <string>
#include <sstream>

#ifdef _INTEL_COMPILER
#pragma warning (disable : 2586)
#pragma warning (disable : 1125)
#pragma warning (disable : 869)
#elif defined(_MSC_VER)
#pragma warning (disable : 4503)
#pragma warning (disable : 4355)
#pragma warning (disable : 4244)
#pragma warning (disable : 4231)
#pragma warning (disable : 4800)
#endif

namespace memoria    {


struct GlobalConstants {
    //defaults:
    static const int KIND = 0;

    //values:
    static const int CACHE_LINE_WIDTH = 64;
};

typedef long long           BigInt;
typedef unsigned long long  UBigInt;
typedef long                Long;
typedef unsigned long       ULong;
typedef int                 Int;
typedef unsigned int        UInt;
typedef short               Short;
typedef unsigned short      UShort;
typedef char                Byte;
typedef unsigned char       UByte;

template <int size> struct PlatformLongHelper;

template <>
struct PlatformLongHelper<4> {
	typedef Int 			PlatformLongType;
	typedef UInt 			PlatformULongType;
};

template <>
struct PlatformLongHelper<8> {
	typedef BigInt 			PlatformLongType;
	typedef UBigInt 		PlatformULongType;
};

typedef PlatformLongHelper<sizeof(void*)>::PlatformLongType 					PlatformLong;
typedef PlatformLongHelper<sizeof(void*)>::PlatformULongType 					PlatformULong;

typedef std::string                                                             String;
typedef const String&                                                      		StringRef;

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

//const bool VALUE_MAP                    = true;
//const bool INDEX_MAP                    = false;

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

struct IdxMap:      public CodeValue<3> {};
struct DFUDS:       public CodeValue<4> {};
struct LOUDS:       public CodeValue<5> {};
struct BlobMap:     public CodeValue<6> {};
struct Array:   	public CodeValue<7> {};



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

template <typename First, typename Second, typename Fird>
struct Tripple {
    typedef First   first;
    typedef Second  second;
    typedef Fird    fird;
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

template <typename Type_>
struct TypeWrapper {
    typedef Type_               Type;
};

template <typename T = void>
class Remover {
    const T* ptr_;
public:
    Remover(const T* ptr): ptr_(ptr) {}
    virtual ~Remover() {
        delete ptr_;
    }
};

struct Typed {
	 virtual ~Typed() throw () {}
};

template <typename SrcType, typename DstType>
union T2TBuf {
	SrcType 	src;
	DstType		dst;
	T2TBuf(SrcType src0): src(src0) {}
};


template <typename DstType, typename SrcType>
DstType T2T(SrcType value) {
	T2TBuf<SrcType, DstType> buf(value);
	return buf.dst;
}

template <typename DstType, typename SrcType>
DstType P2V(SrcType value) {
	T2TBuf<SrcType, DstType*> buf(value);
	return *buf.dst;
}

template <typename DstType, typename SrcType>
const DstType* CP2CP(const SrcType* value) {
	T2TBuf<const SrcType*, const DstType*> buf(value);
	return buf.dst;
}

template <typename DstType, typename SrcType>
DstType& P2R(SrcType value) {
	T2TBuf<SrcType, DstType*> buf(value);
	return *buf.dst;
}

template <typename DstType, typename SrcType>
const DstType& P2CR(const SrcType value) {
	T2TBuf<const SrcType, const DstType*> buf(value);
	return *buf.dst;
}



}

#endif	/* _MEMORIA_VAPI_TYPES_HPP */



