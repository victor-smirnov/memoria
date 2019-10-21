
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


#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/mapped/mapped_hash.hpp>

namespace memoria {
namespace v1 {

template <typename T, typename Arena>
class MappedHashFn;

#define MMA1_MAPPED_HASH_FN(Type)   \
template <typename Arena>           \
class MappedHashFn<Type, Arena> {   \
public:                             \
    MappedHashFn(Arena*) {}         \
    size_t operator()(const Type& value) {  \
        FNVHasher<4> hasher;                \
        append(hasher, value);              \
    }                                       \
}                                           \


MMA1_MAPPED_HASH_FN(uint8_t);
MMA1_MAPPED_HASH_FN(int8_t);

MMA1_MAPPED_HASH_FN(uint16_t);
MMA1_MAPPED_HASH_FN(int16_t);

MMA1_MAPPED_HASH_FN(uint32_t);
MMA1_MAPPED_HASH_FN(int32_t);

MMA1_MAPPED_HASH_FN(uint64_t);
MMA1_MAPPED_HASH_FN(int64_t);

MMA1_MAPPED_HASH_FN(bool);
MMA1_MAPPED_HASH_FN(char);
MMA1_MAPPED_HASH_FN(char16_t);
MMA1_MAPPED_HASH_FN(char32_t);

MMA1_MAPPED_HASH_FN(float);
MMA1_MAPPED_HASH_FN(double);

template <typename T, typename Arena>
class MappedEqualToFn;

#define MMA1_MAPPED_EQUAL_TO_FN(Type)       \
template <typename Arena>                   \
class MappedEqualToFn<Type, Arena> {        \
public:                                     \
    MappedEqualToFn(Arena*) {}              \
                                            \
    template <typename Key>                 \
    bool operator()(const Key& key, const Type& value) {\
        return key == value;                \
    }                                       \
}                                           \

MMA1_MAPPED_EQUAL_TO_FN(uint8_t);
MMA1_MAPPED_EQUAL_TO_FN(int8_t);

MMA1_MAPPED_EQUAL_TO_FN(uint16_t);
MMA1_MAPPED_EQUAL_TO_FN(int16_t);

MMA1_MAPPED_EQUAL_TO_FN(uint32_t);
MMA1_MAPPED_EQUAL_TO_FN(int32_t);

MMA1_MAPPED_EQUAL_TO_FN(uint64_t);
MMA1_MAPPED_EQUAL_TO_FN(int64_t);

MMA1_MAPPED_EQUAL_TO_FN(bool);
MMA1_MAPPED_EQUAL_TO_FN(char);
MMA1_MAPPED_EQUAL_TO_FN(char16_t);
MMA1_MAPPED_EQUAL_TO_FN(char32_t);

MMA1_MAPPED_EQUAL_TO_FN(float);
MMA1_MAPPED_EQUAL_TO_FN(double);

template <typename T, typename Arena>
class MappedPtrHashFn;

template < template<typename, typename, typename> class PtrT, typename T, typename HolderT, typename Arena>
class MappedPtrHashFn<PtrT<T, HolderT, Arena>, Arena> {
    Arena* arena_;
public:
    MappedPtrHashFn(Arena* arena):
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



}}
