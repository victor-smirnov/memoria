
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TYPES_TYPES_HPP
#define _MEMORIA_CORE_TYPES_TYPES_HPP

#include <string>
#include <sstream>
#include <cassert>
#include <stdint.h>

#include <memoria/core/tools/config.hpp>
#include <type_traits>

namespace memoria    {

static const int DEFAULT_BLOCK_SIZE         		= 4096;
static const int PackedTreeBranchingFactor  		= 32;
static const int PackedSeqBranchingFactor   		= 32;
static const int PackedSeqValuesPerBranch   		= 1024;
static const int PackedExintVLETreeValuesPerBranch  = 256;
static const int PackedAllocationAlignment  		= 8;


typedef int64_t             BigInt;
typedef uint64_t            UBigInt;
typedef int32_t             Int;
typedef uint32_t            UInt;
typedef int16_t             Short;
typedef uint16_t            UShort;
typedef char                Byte;
typedef uint8_t             UByte;


namespace internal {
template <int size> struct PlatformLongHelper;

template <>
struct PlatformLongHelper<4> {
    typedef Int             LongType;
    typedef UInt            ULongType;
    typedef Int             SizeTType;
};

template <>
struct PlatformLongHelper<8> {
    typedef BigInt          LongType;
    typedef UBigInt         ULongType;
    typedef BigInt          SizeTType;
};
}

const BigInt CTR_DEFAULT_NAME 			= -1;
const BigInt INITAL_CTR_NAME_COUNTER	= 1000000;

enum {
	CTR_NONE 				= 0,
	CTR_CREATE 				= 1,
	CTR_FIND 				= 1<<1,
	CTR_THROW_IF_EXISTS 	= 1<<2,
};

/**
 * Please note that Long/ULong types are not intended to be used for data page properties.
 * Use types with known size instead.
 */

typedef internal::PlatformLongHelper<sizeof(void*)>::LongType                   Long;
typedef internal::PlatformLongHelper<sizeof(void*)>::ULongType                  ULong;
typedef internal::PlatformLongHelper<sizeof(void*)>::SizeTType                  SizeT;

typedef std::string                                                             String;
typedef const String&                                                           StringRef;

template <typename T, T V> struct ConstValue {
    static const T Value = V;
};

template <UInt Value>
using UIntValue = ConstValue<UInt, Value>;

template <bool Value>
using BoolValue = ConstValue<bool, Value>;

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

    operator BigInt () const {
        return 0;
    }
};

template <typename T> struct TypeHash; // must define Value constant

/*
 * Container type names & profiles
 */

struct BalancedTree {};

struct BTree        {};
struct BSTree       {};
struct Composite    {};
struct Root         {};

template <typename CtrName>
class CtrWrapper 	{};

template <typename Key, typename Value>
struct Map          {};

template <typename Key, typename Value>
struct CMap         {};

//template <typename Key, typename Value>
//struct Map2      	{};



template <typename Key, typename Value>
struct MapProto     {};

template <typename Value>
struct VectorProto  {};

typedef Map<BigInt, BigInt>       Map1;

template <typename Key, Int Indexes = 1>
using Set = Map<Key, EmptyValue>;


typedef Set<BigInt, 1>       Set1;
typedef Set<BigInt, 2>       Set2;

struct DFUDS        {};
struct LOUDS        {};

template <typename Key, typename Value>
struct VectorMap    {};

template <typename T>
struct Vector       {};

struct ASequence	{};

template <Int BitsPerSymbol, bool Dense = true>
struct Sequence	{};

template <bool Dense = true>
using BitVector = Sequence<1, Dense>;

template <typename ChildType = void>
class SmallProfile 	{};

/*
 * End of container type names and profiles
 */


struct NullType {};

struct EmptyType {};
struct IncompleteType;
struct TypeIsNotDefined {};

template <typename Name>
struct TypeNotFound;
struct TypeIsNotDefined;

template <typename FirstType, typename SecondType>
struct Pair {
    typedef FirstType   First;
    typedef SecondType  Second;
};


template <typename T>
struct TypeDef {
    typedef T Type;
};

class NotDefined {};

// For metadata initialization
template <int Order>
struct CtrNameDeclarator: TypeDef<NotDefined> {};

template <typename ... Types>
struct TypeList {};

template <typename T, T ... Values>
struct ValueList {};



template <typename T> struct DeclType {
	typedef T Type;
};

template <typename First, typename Second>
struct ValuePair {

    First   first;
    Second  second;

    ValuePair(const First& f, const Second& s): first(f), second(s) {}
    ValuePair(const First& f): first(f) {}
};

//struct NoParamCtr {};
struct IterEndMark {};

struct SerializationData {
    char* buf;
    Int total;
};

struct DeserializationData {
    const char* buf;
};

enum class WalkDirection {
	UP, DOWN
};

enum class SearchType {LT, LE};
enum class IteratorMode {FORWARD, BACKWARD};
enum class MergeType {NONE, LEFT, RIGHT};
enum class MergePossibility {YES, NO, MAYBE};

extern BigInt DebugCounter;
extern size_t MemBase;


}

#endif
