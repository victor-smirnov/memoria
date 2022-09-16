
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/linked/common/linked_hash.hpp>

#include <memoria/core/datatypes/arena/arena.hpp>

namespace memoria {
namespace arena {

template <typename T>
class DefaultHashFn;


#define MMA_ARENA_DEFAULT_HASH_FN(Type)     \
template <>                                 \
class DefaultHashFn<Type> {                 \
public:                                     \
    uint64_t operator()(const MemorySegment* sgm, const Type& value) {\
        FNVHasher<8> hasher;                \
        append(hasher, value);              \
        return hasher.hash();               \
    }                                       \
}


MMA_ARENA_DEFAULT_HASH_FN(uint8_t);
MMA_ARENA_DEFAULT_HASH_FN(int8_t);

MMA_ARENA_DEFAULT_HASH_FN(uint16_t);
MMA_ARENA_DEFAULT_HASH_FN(int16_t);

MMA_ARENA_DEFAULT_HASH_FN(uint32_t);
MMA_ARENA_DEFAULT_HASH_FN(int32_t);

MMA_ARENA_DEFAULT_HASH_FN(uint64_t);
MMA_ARENA_DEFAULT_HASH_FN(int64_t);

MMA_ARENA_DEFAULT_HASH_FN(bool);
MMA_ARENA_DEFAULT_HASH_FN(char);
MMA_ARENA_DEFAULT_HASH_FN(char16_t);
MMA_ARENA_DEFAULT_HASH_FN(char32_t);

MMA_ARENA_DEFAULT_HASH_FN(float);
MMA_ARENA_DEFAULT_HASH_FN(double);


template <typename Type>
class DefaultHashFn<SegmentPtr<Type>> {
public:
    uint64_t operator()(const MemorySegment* sgm, const SegmentPtr<Type>& ptr)
    {
        FNVHasher<8> hasher;
        const Type* tt = ptr.read(sgm);
        tt->hash_to(sgm, hasher);
        return hasher.hash();
    }
};



template <typename Type>
class DefaultEqualToFn {
    template <typename Key>
    bool operator()(const MemorySegment*, const Key& key, const Type& stored) {
        return key == stored;
    }
};


template <typename Type>
class DefaultEqualToFn<SegmentPtr<Type>> {
    template <typename Key>
    bool operator()(const MemorySegment* sgm, const Key& key, const Type& stored) {
        return key.equals_to(sgm, stored);
    }
};



#define MMA_ARENA_DEFAULT_EQUAL_TO_FN(Type) \
template <>                                 \
class DefaultEqualToFn<Type> {              \
public:                                     \
    template <typename Key>                 \
    bool operator()(const MemorySegment*, const Key& key, const Type& stored) {\
        return key == stored;               \
    }                                       \
}

MMA_ARENA_DEFAULT_EQUAL_TO_FN(uint8_t);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(int8_t);

MMA_ARENA_DEFAULT_EQUAL_TO_FN(uint16_t);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(int16_t);

MMA_ARENA_DEFAULT_EQUAL_TO_FN(uint32_t);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(int32_t);

MMA_ARENA_DEFAULT_EQUAL_TO_FN(uint64_t);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(int64_t);

MMA_ARENA_DEFAULT_EQUAL_TO_FN(bool);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(char);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(char16_t);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(char32_t);

MMA_ARENA_DEFAULT_EQUAL_TO_FN(float);
MMA_ARENA_DEFAULT_EQUAL_TO_FN(double);



}}
