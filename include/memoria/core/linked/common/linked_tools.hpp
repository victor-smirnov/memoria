
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/linked/common/linked_hash.hpp>

namespace memoria {
namespace ld {

template <typename T, typename Arena>
class MappedHashFn;

#define MMA_MAPPED_HASH_FN(Type)   \
template <typename Arena>           \
class MappedHashFn<Type, Arena> {   \
public:                             \
    MappedHashFn(Arena*) {}         \
    uint32_t operator()(const Type& value) {  \
        FNVHasher<4> hasher;                \
        append(hasher, value);              \
        return hasher.hash();               \
    }                                       \
}                                           \


MMA_MAPPED_HASH_FN(uint8_t);
MMA_MAPPED_HASH_FN(int8_t);

MMA_MAPPED_HASH_FN(uint16_t);
MMA_MAPPED_HASH_FN(int16_t);

MMA_MAPPED_HASH_FN(uint32_t);
MMA_MAPPED_HASH_FN(int32_t);

MMA_MAPPED_HASH_FN(uint64_t);
MMA_MAPPED_HASH_FN(int64_t);

MMA_MAPPED_HASH_FN(bool);
MMA_MAPPED_HASH_FN(char);
MMA_MAPPED_HASH_FN(char16_t);
MMA_MAPPED_HASH_FN(char32_t);

MMA_MAPPED_HASH_FN(float);
MMA_MAPPED_HASH_FN(double);

template <typename T, typename Arena>
class MappedEqualToFn;

#define MMA_MAPPED_EQUAL_TO_FN(Type)       \
template <typename Arena>                   \
class MappedEqualToFn<Type, Arena> {        \
public:                                     \
    MappedEqualToFn(const Arena*) {}        \
                                            \
    template <typename Key>                 \
    bool operator()(const Key& key, const Type& value) {\
        return key == value;                \
    }                                       \
}                                           \

MMA_MAPPED_EQUAL_TO_FN(uint8_t);
MMA_MAPPED_EQUAL_TO_FN(int8_t);

MMA_MAPPED_EQUAL_TO_FN(uint16_t);
MMA_MAPPED_EQUAL_TO_FN(int16_t);

MMA_MAPPED_EQUAL_TO_FN(uint32_t);
MMA_MAPPED_EQUAL_TO_FN(int32_t);

MMA_MAPPED_EQUAL_TO_FN(uint64_t);
MMA_MAPPED_EQUAL_TO_FN(int64_t);

MMA_MAPPED_EQUAL_TO_FN(bool);
MMA_MAPPED_EQUAL_TO_FN(char);
MMA_MAPPED_EQUAL_TO_FN(char16_t);
MMA_MAPPED_EQUAL_TO_FN(char32_t);

MMA_MAPPED_EQUAL_TO_FN(float);
MMA_MAPPED_EQUAL_TO_FN(double);

template <typename T, typename Arena>
class LinkedPtrHashFn;

template < template<typename, typename, typename> class PtrT, typename T, typename HolderT, typename Arena>
class LinkedPtrHashFn<PtrT<T, HolderT, Arena>, Arena> {
    const Arena* arena_;
public:
    LinkedPtrHashFn(const Arena* arena):
        arena_(arena)
    {}

    size_t operator()(const PtrT<T, HolderT, Arena>& ptr)
    {
        FNVHasher<4> hasher;
        append(hasher, *ptr.get(arena_));
        return hasher.hash();
    }

    template <typename TT>
    size_t operator()(const TT& value)
    {
        FNVHasher<4> hasher;
        append(hasher, value);
        return hasher.hash();
    }
};


namespace linked_ {

    template <typename T> struct DeepCopyHelper;

    template <typename T, typename HolderT, typename Arena>
    struct DeepCopyHelper<LinkedPtr<T, HolderT, Arena>>
    {
        using PtrT = LinkedPtr<T, HolderT, Arena>;

        template <typename AddressMapping>
        static PtrT deep_copy(Arena* dst, const Arena* src, PtrT ptr, AddressMapping& address_mapping)
        {
            auto ii = address_mapping.find(ptr.get());
            if (MMA_UNLIKELY(ii != address_mapping.end()))
            {
                return ii->second;
            }
            else {
                PtrT new_ptr = ptr.get(src)->deep_copy_to(dst, address_mapping);
                address_mapping[ptr.get()] = new_ptr;
                return new_ptr;
            }
        }
    };

    template <typename T>
    struct DeepCopyHelper
    {
        template <typename Arena, typename AddressMapping>
        static const T& deep_copy(Arena* dst, const Arena* src, const T& value, AddressMapping& address_mapping)
        {
            return value;
        }
    };
}

}}
