
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

#include <memoria/v1/core/config.hpp>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

#include <string>
#include <sstream>
#include <cassert>
#include <cstdint>
#include <exception>

#include <unicode/utypes.h>

#include <type_traits>
#include <tuple>

namespace memoria {
namespace v1 {

static constexpr int DEFAULT_BLOCK_SIZE                 = 8192;
static constexpr int PackedTreeBranchingFactor          = 32;
static constexpr int PackedSeqBranchingFactor           = 32;
static constexpr int PackedSeqValuesPerBranch           = 1024;
static constexpr int PackedTreeExintVPB                 = 256;
static constexpr int PackedTreeEliasVPB                 = 1024;
static constexpr int PackedAllocationAlignment          = 8;

static constexpr size_t MaxRLERunLength                 = 0x7FFFFFF;

enum class PackedSizeType {FIXED, VARIABLE};

namespace internal {
    template <int size> struct PlatformLongHelper;

    template <>
    struct PlatformLongHelper<4> {
        typedef int32_t             LongType;
        typedef uint32_t            ULongType;
        typedef int32_t             SizeTType;
    };

    template <>
    struct PlatformLongHelper<8> {
        typedef int64_t          LongType;
        typedef uint64_t         ULongType;
        typedef int64_t          SizeTType;
    };
}

enum class CtrPageType {ROOT, LEAF, INTERNAL, ROOT_LEAF};

inline bool is_root(CtrPageType type) {
    return type == CtrPageType::ROOT || type == CtrPageType::ROOT_LEAF;
}

inline bool is_leaf(CtrPageType type) {
    return type == CtrPageType::LEAF || type == CtrPageType::ROOT_LEAF;
}

inline bool is_branch(CtrPageType type) {
    return type == CtrPageType::ROOT || type == CtrPageType::INTERNAL;
}



enum {
    CTR_NONE                = 0,
    CTR_CREATE              = 1,
    CTR_FIND                = 1 << 1
};

/**
 * Please note that Long/ULong types are not intended to be used for data page properties.
 * Use types with known size instead.
 */

typedef internal::PlatformLongHelper<sizeof(void*)>::LongType                   Long;
typedef internal::PlatformLongHelper<sizeof(void*)>::ULongType                  ULong;
typedef internal::PlatformLongHelper<sizeof(void*)>::SizeTType                  SizeT;



template <typename T, T V> struct ConstValue {
    static constexpr T Value = V;

    constexpr operator T() const noexcept {return Value;}
    constexpr T operator()() const noexcept {return Value;}
};

template <uint32_t Value>
using UInt32Value = ConstValue<uint32_t, Value>;

template <uint64_t Value>
using UInt64Value = ConstValue<uint64_t, Value>;

template <int32_t Value>
using IntValue = ConstValue<int32_t, Value>;


template <bool Value>
using BoolValue = ConstValue<bool, Value>;


template <typename...> using VoidT = void;

class EmptyValue {
public:
    EmptyValue() {}
    EmptyValue(const int32_t) {}
    EmptyValue(const int64_t) {}
    EmptyValue(const EmptyValue& other) {}
    EmptyValue& operator=(const EmptyValue& other) {
        return *this;
    }

    template <typename T>
    operator T () {
        return 0;
    }

    operator int64_t () const {
        return 0;
    }
};

template <typename T> struct TypeHash; // must define Value constant

template <typename T>
constexpr uint64_t TypeHashV = TypeHash<T>::Value;

template <typename ... Types>
struct TypeList {
    constexpr TypeList() = default;
};

template <typename ... Types>
using TL = TypeList<Types...>;


template <typename T, T ... Values>
struct ValueList {
    constexpr ValueList() = default;
};


template <int32_t... Values>
using IntList = ValueList<int32_t, Values...>;

template <uint32_t... Values>
using UInt32List = ValueList<uint32_t, Values...>;

template <uint64_t... Values>
using UInt64List = ValueList<uint64_t, Values...>;


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
struct Multimap     {};

template <typename Key, typename Value>
struct CowMap       {};

template <typename Key, typename Value, PackedSizeType SizeType>
struct Table        {};

template <typename Key>
struct Set          {};

template <typename T>
struct Vector       {};

template <int32_t BitsPerSymbol, bool Dense = true>
struct Sequence {};

template <bool Dense = true>
using BitVector = Sequence<1, Dense>;

template <typename ChildType = void>
class DefaultProfile  {};

enum class Granularity  {Bit, int8_t};
enum class Indexed      {No, Yes};

template <
    typename LblType,
    Indexed indexed         = Indexed::No
>
struct FLabel       {};

template <
    int32_t BitsPerSymbol
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

template <Granularity granularity, typename T = int64_t>
struct VLen {};

template <Granularity gr = Granularity::int8_t>
using CMap = Map<VLen<gr>, int64_t>;

template <Granularity gr = Granularity::int8_t>
using CCMap = Map<VLen<gr>, VLen<gr>>;


// Database-specific containers
struct BlobStore        {};
struct EdgeMap          {};
struct InvertedIndex    {};
struct ObjectStore      {};
struct RowStore         {};
struct ScopedDictionary {};
struct UpdateLog        {};

struct UBytes;

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

struct EmptyType1 {};
struct EmptyType2 {};

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
    int32_t total;

    SerializationData(): buf(nullptr), total(0) {}
};

struct DeserializationData {
    const char* buf;
};

enum class WalkDirection {
    UP, DOWN
};

enum class WalkCmd {
    FIRST_LEAF, LAST_LEAF, THE_ONLY_LEAF, FIX_TARGET, NONE, PREFIXES, REFRESH
};


enum class UpdateType {
    SET, ADD
};

enum class SearchType {LT, LE, GT, GE};
enum class IteratorMode {FORWARD, BACKWARD};
enum class MergeType {NONE, LEFT, RIGHT};
enum class MergePossibility {YES, NO, MAYBE};

enum class LeafDataLengthType {FIXED, VARIABLE};

template <typename T>
struct TypeP {
    using Type = T;
};

class NoParamCtr {};


class VLSelector {};
class FLSelector {};

enum class SplitStatus {NONE, LEFT, RIGHT, UNKNOWN};

class SplitResult {
    SplitStatus type_;
    int32_t stream_idx_;
public:
    SplitResult(SplitStatus type, int32_t stream_idx):
        type_(type), stream_idx_(stream_idx)
    {}

    SplitResult(SplitStatus type): type_(type), stream_idx_() {}

    SplitStatus type() const {return type_;}
    int32_t stream_idx() const {return stream_idx_;}
};


template <typename PkdStruct>
struct IndexesSize {
    static const int32_t Value = PkdStruct::Indexes;
};


extern int64_t DebugCounter;
extern int64_t DebugCounter1;
extern int64_t DebugCounter2;

template <typename T>
using IL = std::initializer_list<T>;

template <typename List>  struct AsTupleH;

template <typename List>
using AsTuple = typename AsTupleH<List>::Type;

template <typename... Types>
struct AsTupleH<TL<Types...>> {
    using Type = std::tuple<Types...>;
};


template <typename T>
struct HasType {
    using Type = T;
};

template <typename T, T V_>
struct HasValue {
    static constexpr T Value = V_;
};


namespace _ {
    template <typename T, bool Flag, typename... AdditionalTypes>
    struct FailIfT {
        static_assert(!Flag, "Template failed");
        using Type = T;
    };

}

template <bool Flag, typename T, typename... AdditionalTypes>
using FailIf = typename v1::_::FailIfT<T, Flag, AdditionalTypes...>::Type;

template <bool Flag, int32_t V, typename... AdditionalTypes>
using FailIfV = typename v1::_::FailIfT<IntValue<V>, Flag, AdditionalTypes...>::Type;

template <typename T, typename T1 = int32_t, T1 V = T1{}>
struct FakeValue: HasValue<T1, V> {};



template <typename T1, typename T2>
constexpr bool compare_gt(T1&& first, T2&& second) {
    return first > second;
}

template <typename T1, typename T2>
constexpr bool compare_eq(T1&& first, T2&& second) {
    return first == second;
}

template <typename T1, typename T2>
constexpr bool compare_lt(T1&& first, T2&& second) {
    return first < second;
}

template <typename T1, typename T2>
constexpr bool compare_ge(T1&& first, T2&& second) {
    return first >= second;
}

template <typename T1, typename T2>
constexpr bool compare_le(T1&& first, T2&& second) {
    return first <= second;
}


template <typename T> class ValueCodec;
template <typename T> struct FieldFactory;

template <typename T>
class ValuePtrT1 {
    const T* addr_;
    size_t length_;
public:
    ValuePtrT1(): addr_(), length_() {}
    ValuePtrT1(const T* addr, size_t length): addr_(addr), length_(length) {}

    const T* addr() const {return addr_;}
    size_t length() const {return length_;}
};

template <typename T>
class ValuePtrT2 {
    const T* addr_;
    size_t offset_;
    size_t length_;
public:
    ValuePtrT2(): addr_(), offset_(0), length_() {}
    ValuePtrT2(const T* addr, size_t offset, size_t length): addr_(addr), offset_(offset), length_(length) {}

    const T* addr() const {return addr_;}
    size_t length() const {return length_;}
    size_t offset() const {return offset_;}
};


namespace {
    template <typename T> struct Void {
        using Type = void;
    };

    template <typename T>
    struct IsCompleteHelper {
        template <typename U>
        static auto test(U*)  -> std::integral_constant<bool, sizeof(U) == sizeof(U)>;
        static auto test(...) -> std::false_type;
        using Type = decltype(test((T*)0));
    };
}

template <typename T>
struct IsComplete : IsCompleteHelper<T>::Type {};


template <typename T>
struct HasValueCodec: HasValue<bool, IsComplete<ValueCodec<T>>::type::value> {};

template <typename T>
struct HasFieldFactory: HasValue<bool, IsComplete<FieldFactory<T>>::type::value> {};

template <typename T>
struct IsExternalizable: HasValue<bool, HasValueCodec<T>::Value || HasFieldFactory<T>::Value> {};

template <typename T> struct IOBufferAdapter;

struct Referenceable {
    virtual ~Referenceable() {}
};

struct ReferenceableNoExcept {
    virtual ~ReferenceableNoExcept() noexcept {}
};


enum class ByteOrder {
    BIG, LITTLE
};

enum class MemoryAccess {
    MMA_ALIGNED, MMA_UNALIGNED
};


template <typename T> struct TypeTag {};


enum class MMA1_NODISCARD OpStatus: int32_t {
    OK = 0, FAIL = 1
};

static inline OpStatus& operator<<=(OpStatus& s1, OpStatus s2) {
    s1 = static_cast<OpStatus>(static_cast<int32_t>(s1) + static_cast<int32_t>(s2));
    return s1;
}

template <typename T>
class MMA1_NODISCARD OpStatusT {
    T value_;
    OpStatus status_;
public:
    OpStatusT(T value): value_(std::move(value)), status_(OpStatus::OK) {}
    OpStatusT(): value_{}, status_(OpStatus::FAIL) {}
    OpStatusT(OpStatus s): value_{}, status_(s) {}

    OpStatus status() const {return status_;}
    const T& value() const {return value_;}
};

template <typename T>
static inline OpStatus& operator<<=(OpStatus& s1, OpStatusT<T> s2) {
    s1 = static_cast<OpStatus>(static_cast<int32_t>(s1) + static_cast<int32_t>(s2.status()));
    return s1;
}

static inline bool isFail(OpStatus status) {return status != OpStatus::OK;}
static inline bool isFail(int32_t status) {return status < 0;}
static inline bool isFail(void) {return false;}

static inline bool isOk(OpStatus status) {return status == OpStatus::OK;}

template <typename T>
static inline bool isFail(const T* ptr) {return ptr == nullptr;}

template <typename T>
bool isFail(const OpStatusT<T>& op_status) {
    return isFail(op_status.status());
}

}}
