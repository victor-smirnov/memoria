
// Copyright Victor Smirnov 2011-2013.
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

static const int DEFAULT_BLOCK_SIZE                 = 4096;
static const int PackedTreeBranchingFactor          = 32;
static const int PackedSeqBranchingFactor           = 32;
static const int PackedSeqValuesPerBranch           = 1024;
static const int PackedTreeExintVPB                 = 256;
static const int PackedTreeEliasVPB                 = 1024;
static const int PackedAllocationAlignment          = 8;


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

const BigInt CTR_DEFAULT_NAME           = -1;
const BigInt INITAL_CTR_NAME_COUNTER    = 1000000;

enum {
    CTR_NONE                = 0,
    CTR_CREATE              = 1,
    CTR_FIND                = 1<<1,
    CTR_THROW_IF_EXISTS     = 1<<2,
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


template <typename ... Types>
struct TypeList {};

template <typename T, T ... Values>
struct ValueList {};






/*
 * Container type names & profiles
 */

struct BT {};



struct Composite    {};
struct Root         {};

template <typename CtrName>
class CtrWrapper    {};

template <typename Key, typename Value>
struct Map          {};

template <typename Key, typename Value>
struct Map2         {};

template <typename K, typename V>
using DblMap = Map<K, Map<K, V>>;

template <typename K, typename V>
using DblMap2 = Map2<K, Map2<K, V>>;

template <
	Int Indexes,
	typename Key,
	typename Value,
	typename LabelsList 		= TypeList<>,
	typename HiddenLabelsList 	= TypeList<>
>
struct MetaMap      {};



template <typename Value>
struct VectorProto  {};

typedef Map<BigInt, BigInt>       Map1;

template <typename Key, Int Indexes = 1>
using Set = Map<Key, EmptyValue>;


typedef Set<BigInt, 1>       Set1;
typedef Set<BigInt, 2>       Set2;

template <typename Key, typename Value>
struct VectorMap    {};

template <typename T>
struct Vector       {};

struct ASequence    {};

template <Int BitsPerSymbol, bool Dense = true>
struct Sequence {};

template <bool Dense = true>
using BitVector = Sequence<1, Dense>;

template <typename ChildType = void>
class SmallProfile  {};

template <typename ChildType = void>
class FileProfile  {};

enum class Granularity  {Bit, Byte};
enum class Indexed      {No, Yes};

template <
    typename LblType,
    Indexed indexed         = Indexed::No
>
struct FLabel       {};

template <
    Int BitsPerSymbol
>
struct FBLabel      {};

template <
    typename LblType,
    Granularity granularity = Granularity::Bit,
    Indexed indexed         = Indexed::No
>
struct VLabel       {};


template <typename... LabelDescriptors>
struct LabeledTree  {};


struct WT           {};
struct VTree        {};

template <Granularity gr = Granularity::Byte>
struct CMap         {};

// A map with marked K/V pairs
template <typename Key, typename Value, Int BitsPerMark = 1>
struct MrkMap		{};

// A map with marked K/V pairs
template <typename Key, typename Value, Int BitsPerMark = 1>
struct MrkMap2		{};

// A map with marked K/V pairs, with search over marks
template <typename Key, typename Value, Int BitsPerMark = 1>
struct SMrkMap		{};

template <typename K, typename V, Int BitsPerMark>
using DblMrkMap = Map<K, MrkMap<K, V, BitsPerMark>>;

template <typename K, typename V, Int BitsPerMark>
using DblMrkMap2 = Map2<K, MrkMap2<K, V, BitsPerMark>>;


template <Granularity granularity, typename T = BigInt>
struct VLen {};

// Placeholder type to be used in place of Page IDs
struct IDType {};


/*
 * End of container type names and profiles
 */

/*
 * Prototype names
 */

class Tree {};

/*
 * End of prototype names
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









template <typename T> struct DeclType {
    typedef T Type;
};

template <typename First, typename Second>
struct ValuePair {

    First   first;
    Second  second;

    ValuePair(const First& f, const Second& s): first(f), second(s) {}
    ValuePair(const First& f): first(f) {}
    ValuePair() {}
};


struct IterEndMark {};

struct SerializationData {
    char* buf;
    Int total;

    SerializationData(): buf(nullptr), total(0) {}
};

struct DeserializationData {
    const char* buf;
};

enum class WalkDirection {
    UP, DOWN
};

enum class SearchType {LT, LE, GT, GE};
enum class IteratorMode {FORWARD, BACKWARD};
enum class MergeType {NONE, LEFT, RIGHT};
enum class MergePossibility {YES, NO, MAYBE};

class NoParamCtr {};

extern BigInt DebugCounter;
extern BigInt DebugCounter1;
extern BigInt DebugCounter2;
extern size_t MemBase;


}

#endif
