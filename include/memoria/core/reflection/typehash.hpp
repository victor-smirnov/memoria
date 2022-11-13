
// Copyright 2011-2022 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/static_md5.hpp>

#include <tuple>

namespace memoria {

template <typename T>
struct TypeDescriptor: HasU64Value<0> {};

class ShortTypeCode final {
    uint64_t value_;

    constexpr ShortTypeCode(uint64_t v):
        value_(v)
    {}


public:
    ShortTypeCode() = default;

    constexpr uint64_t u64() const {
        return value_;
    }

    constexpr bool operator==(ShortTypeCode other) const noexcept {
        return value_ == other.value_;
    }

    constexpr bool operator!=(ShortTypeCode other) const noexcept {
        return value_ != other.value_;
    }

    constexpr bool operator<(ShortTypeCode other) const noexcept {
        return value_ < other.value_;
    }

    // Only 56 bit of code matters
    // 0 = 'null' code (0-sized)
    static constexpr uint64_t code_len(uint64_t value)
    {
        size_t len = 7;
        uint64_t mask = 0xFF000000000000;
        for (; len > 0; len--, mask >>= 8) {
            if (value & mask) {
                return len;
            }
        }

        return len;
    }

    constexpr size_t code_len() const {
        return value_ & 0x7ull;
    }

    constexpr size_t full_code_len() const {
        return code_len() + 1;
    }


    constexpr uint64_t descriptor() const {
        return (value_ >> 3) & 0x1Full;
    }

    template <typename T>
    static constexpr ShortTypeCode of() {
        return ShortTypeCode::with_descriptor(TypeHash<T>::Value, TypeDescriptor<T>::Value);
    }

    constexpr bool is_null() const {
        return !(value_ & 0x7);
    }

    constexpr bool is_not_null() const {
        return (value_ & 0x7);
    }

    static  constexpr ShortTypeCode nullv() {
        return ShortTypeCode(0);
    }

    static constexpr ShortTypeCode of_raw(uint64_t code) {
        return ShortTypeCode(code);
    }

    static constexpr ShortTypeCode of_object(uint64_t type_hash) {
        return ShortTypeCode((type_hash << 8) | code_len(type_hash));
    }

    static constexpr ShortTypeCode with_descriptor(uint64_t type_hash, uint64_t descriptor) {
        return ShortTypeCode((type_hash << 8) | code_len(type_hash) | ((descriptor & 0x1Full) << 3));
    }
};

// FIXME: remove after removing LDDocument
static constexpr uint8_t SHORT_TYPEHASH_LENGTH_BASE = 250;


struct TypeHashes {
    enum {SCALAR = 1, ARRAY, CONST_VALUE};
};

// Type Code value 0 means 'no code'

template <> struct TypeHash<int8_t>:    UInt64Value<1> {};
template <> struct TypeHash<uint8_t>:   UInt64Value<2> {};
template <> struct TypeHash<int16_t>:   UInt64Value<3> {};
template <> struct TypeHash<uint16_t>:  UInt64Value<4> {};
template <> struct TypeHash<int32_t>:   UInt64Value<5> {};
template <> struct TypeHash<uint32_t>:  UInt64Value<6> {};
template <> struct TypeHash<int64_t>:   UInt64Value<7> {};
template <> struct TypeHash<uint64_t>:  UInt64Value<8> {};
template <> struct TypeHash<float>:     UInt64Value<9> {};
template <> struct TypeHash<double>:    UInt64Value<10> {};
template <> struct TypeHash<void>:      UInt64Value<11> {};

// Core datatypes: 20 - 39
// LinkedData: 40 - 44
// Reserved for Linked Data : 249 - 255

template <
    template <typename> class Profile,
    typename T
>
struct TypeHash<Profile<T>> {
    // FIXME need template assigning unique code to the each profile level
    using VList = UInt64List<100, TypeHash<T>::Value>;
    static constexpr uint64_t Value = md5::Md5Sum<VList>::Type::Value64;
};





template <uint64_t Base, uint64_t ... Values>
static constexpr uint64_t HashHelper = md5::Md5Sum<UInt64List<Base, Values...>>::Type::Value64;


template <typename T, T V>
struct TypeHash<ConstValue<T, V>> {
    static const uint64_t Value = HashHelper<TypeHashV<T>, TypeHashes::CONST_VALUE, V>;
};



template <typename T, size_t Size>
struct TypeHash<T[Size]> {
    static const uint64_t Value = HashHelper<TypeHashV<T>, TypeHashes::ARRAY, Size>;
};


template <typename... List>
struct TypeHash<TypeList<List...>> {
private:
    using ValueList       = typename TypeToValueList<TypeList<List...>>::Type;
    using TaggedValueList = MergeValueLists<UInt64Value<2000>, ValueList>;
public:

    static const uint64_t Value = md5::Md5Sum<TaggedValueList>::Result::Value64;
};

template <typename... List>
struct TypeHash<std::tuple<List...>> {
private:
    using ValueList       = typename TypeToValueList<TypeList<List...>>::Type;
    using TaggedValueList = MergeValueLists<UInt64Value<2001>, ValueList>;
public:

    static const uint64_t Value = md5::Md5Sum<TaggedValueList>::Result::Value64;
};


}
