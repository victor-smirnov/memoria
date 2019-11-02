
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
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/core/linked/common/arena.hpp>
#include <memoria/v1/core/linked/common/linked_string.hpp>
#include <memoria/v1/core/linked/common/linked_dyn_vector.hpp>
#include <memoria/v1/core/linked/common/linked_map.hpp>
#include <memoria/v1/core/linked/common/linked_set.hpp>

namespace memoria {
namespace v1 {

struct SDN2Header {
    uint32_t version;
};

using SDN2Arena     = LinkedArena<SDN2Header, uint32_t>;
using SDN2ArenaBase = typename SDN2Arena::ArenaBase;
using SDN2PtrHolder = SDN2ArenaBase::PtrHolderT;
using LDDValueTag   = uint16_t;

template <typename T>
using SDN2Ptr = typename SDN2Arena::template PtrT<T>;

namespace sdn2_ {

    using PtrHolder = SDN2ArenaBase::PtrHolderT;

    using ValueMap = LinkedMap<
        SDN2Ptr<U8LinkedString>,
        PtrHolder,
        SDN2ArenaBase,
        LinkedPtrHashFn,
        LinkedStringPtrEqualToFn
    >;

    using StringSet = LinkedSet<
        SDN2Ptr<U8LinkedString>,
        SDN2ArenaBase,
        LinkedPtrHashFn,
        LinkedStringPtrEqualToFn
    >;

    using MapState = typename ValueMap::State;

    using Array = LinkedDynVector<PtrHolder, SDN2ArenaBase>;
    using ArrayState = typename Array::State;

    struct TypedValueState {
        PtrHolder type_ptr;
        PtrHolder value_ptr;
    };

    struct TypeDeclState;

    using TypeDeclPtr = SDN2Ptr<TypeDeclState>;

    struct TypeDeclState {
        SDN2Ptr<U8LinkedString> name;
        SDN2Ptr<LinkedDynVector<TypeDeclPtr, SDN2ArenaBase>> type_params;
        SDN2Ptr<LinkedDynVector<PtrHolder, SDN2ArenaBase>> ctr_params;
    };

    struct DocumentState {
        PtrHolder value;
        SDN2Ptr<LinkedVector<TypeDeclPtr>> type_directory;
        SDN2Ptr<sdn2_::StringSet::State> strings;
    };

    static inline void make_indent(std::ostream& out, size_t tabs) {
        for (size_t c = 0; c < tabs; c++) {
            out << "  ";
        }
    }
}



class LDDArray;

class LDDValue;
class LDDMap;
class LDTypeDeclaration;
class LDTypeName;
class LDDataTypeParam;
class LDDataTypeCtrArg;
class LDDocument;
class LDString;
class LDIdentifier;

class LDDocumentBuilder;


template <typename T> struct LDDValueTraits;

template <>
struct LDDValueTraits<LDString> {
    static constexpr LDDValueTag ValueTag = 1;
};

template <>
struct LDDValueTraits<int64_t> {
    static constexpr LDDValueTag ValueTag = 2;
};

template <>
struct LDDValueTraits<double> {
    static constexpr LDDValueTag ValueTag = 3;
};

template <>
struct LDDValueTraits<LDDMap> {
    static constexpr LDDValueTag ValueTag = 5;
};

template <>
struct LDDValueTraits<LDDArray> {
    static constexpr LDDValueTag ValueTag = 6;
};

template <>
struct LDDValueTraits<LDTypeDeclaration> {
    static constexpr LDDValueTag ValueTag = 7;
};

template <>
struct LDDValueTraits<LDIdentifier> {
    static constexpr LDDValueTag ValueTag = 8;
};

}}
