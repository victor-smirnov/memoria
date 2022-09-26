
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

#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/memory/shared_ptr.hpp>

#include <typeinfo>

namespace memoria {

U8String normalize_type_declaration(U8StringView type_decl);
UID256 compute_full_type_hash(U8StringView type_decl);
UID256 compute_full_type_hash_normalized(U8StringView type_decl);

namespace hermes {
class HermesDocView;
}

class TypeReflection {
public:
    virtual ~TypeReflection() noexcept = default;

    virtual const std::type_info& type_info() const noexcept = 0;
    virtual U8String str() = 0;

    // Not all types may have short type hash
    // TypeHash<T>
    virtual uint64_t shot_type_hash() const noexcept = 0;



    virtual void hermes_stringify_value(
            void* ptr,
            hermes::HermesDocView* doc,
            ViewPtrHolder* ref_holder,

            std::ostream& out,
            hermes::DumpFormatState& state,
            hermes::DumpState& dump_state
    ) = 0;

    virtual bool hermes_is_simple_layout(
            void* ptr,
            hermes::HermesDocView* doc,
            ViewPtrHolder* ref_holder
    ) = 0;

};


template <typename T>
class TypeReflectionImplBase: public TypeReflection {
public:
    TypeReflectionImplBase() {}

    virtual const std::type_info& type_info() const noexcept {
        return typeid(T);
    }

    virtual U8String str() {
        return TypeNameFactory<T>::name();
    }

    virtual bool hermes_is_simple_layout(
            void* ptr,
            hermes::HermesDocView* doc,
            ViewPtrHolder* ref_holder
    ) {
        MEMORIA_MAKE_GENERIC_ERROR("hermes_is_simple_layout() is not implemented for type {}", str()).do_throw();
    }
};

template <typename T>
class TypehashTypeReflectionImplBase: public TypeReflectionImplBase<T> {
public:
    TypehashTypeReflectionImplBase() {}

    // Not all types may have short type hash
    // TypeHash<T>
    virtual uint64_t shot_type_hash() const noexcept {
        return TypeHashV<T>;
    };
};



TypeReflection& get_type_reflection(uint64_t short_type_hash);

bool has_type_reflection(uint64_t short_type_hash);

void register_type_reflection(std::unique_ptr<TypeReflection> type_reflection);

}
