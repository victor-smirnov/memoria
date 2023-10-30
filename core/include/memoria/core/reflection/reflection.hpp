
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
#include <memoria/core/hermes/serialization.hpp>
#include <memoria/core/memory/shared_ptr.hpp>

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/core/tools/any.hpp>


#include <typeinfo>
#include <functional>


namespace memoria {

U8String normalize_type_declaration(U8StringView type_decl);
UID256 compute_full_type_hash(U8StringView type_decl);
UID256 compute_full_type_hash_normalized(U8StringView type_decl);

struct IDatatypeConverter {
    virtual ~IDatatypeConverter() noexcept;

    virtual hermes::Object convert(const void* view) const = 0;
};


class TypeReflection: public std::enable_shared_from_this<TypeReflection> {
public:
    virtual ~TypeReflection() noexcept = default;

    virtual std::shared_ptr<const TypeReflection> self() const {
        return this->shared_from_this();
    }

    virtual std::shared_ptr<TypeReflection> self() {
        return this->shared_from_this();
    }

    virtual const std::type_info& type_info() const noexcept = 0;
    virtual U8String str() const = 0;

    // Not all types may have short type hash
    // TypeHash<T>
    virtual ShortTypeCode shot_type_hash() const noexcept {
        MEMORIA_MAKE_GENERIC_ERROR("Method shot_type_hash() is not implemented for {}", str()).do_throw();
    }

    virtual void hermes_stringify_value(
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,

            std::ostream& out,
            hermes::DumpFormatState& state
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method hermes_stringify_value(...) is not implemented for {}", str()).do_throw();
    }

    virtual bool hermes_is_simple_layout(
            LWMemHolder* ref_holder,
            void* ptr
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Method hermes_is_simple_layout(...) is not implemented for {}", str()).do_throw();
    }

    template <typename T>
    T* deep_copy(
            T* src,
            hermes::DeepCopyState& dedup
    ) const {
        return reinterpret_cast<T*>(
            deep_copy_to(src, dedup)
        );
    }

    virtual void* deep_copy_to(
            void*,
            hermes::DeepCopyState&
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Deep copy is not implemented for class {}", str()).do_throw();
    }

    virtual bool is_convertible_to(ShortTypeCode) const {
        return false;
    }

    virtual bool is_convertible_to_plain_string() const {
        return false;
    }

    virtual bool is_convertible_from_plain_string() const {
        return false;
    }

    virtual hermes::Object datatype_convert_to(
            LWMemHolder* ref_holder,
            ShortTypeCode target_tag,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr
    ) const ;

    virtual hermes::Object datatype_convert_from_plain_string(U8StringView str) const;

    virtual U8String convert_to_plain_string(
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr
    ) const;

    virtual bool hermes_comparable() const {
        return false;
    }

    virtual bool hermes_comparable_with(ShortTypeCode tag) const {
        return false;
    }

    virtual bool hermes_equals_comparable_with(ShortTypeCode tag) const {
        return false;
    }

    virtual Any create_cxx_instance(const hermes::Datatype&) {
        MEMORIA_MAKE_GENERIC_ERROR("{} is not a datatype", str()).do_throw();
    }

    virtual int32_t hermes_compare(
            LWMemHolder*,
            hermes::ValueStorageTag,
            hermes::ValueStorage&,
            LWMemHolder*,
            hermes::ValueStorageTag,
            hermes::ValueStorage&
    ) const
    {
        MEMORIA_MAKE_GENERIC_ERROR("Objects of the same type {} are not Hermes-comparable", str()).do_throw();
    }

    virtual bool hermes_equals(
            LWMemHolder*,
            hermes::ValueStorageTag,
            hermes::ValueStorage&,
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag,
            hermes::ValueStorage&
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Objects of type {} are not Hermes-equals-comparable", str()).do_throw();
    }

    virtual hermes::Object import_value(
            LWMemHolder*,
            hermes::ValueStorageTag, hermes::ValueStorage&
    ) const;

    virtual bool hermes_is_ptr_embeddable() const {
        return false;
    }

    virtual bool hermes_embed(arena::ERelativePtr& dst, const hermes::TaggedValue&) const {
        return false;
    }

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_wrapper(
            LWMemHolder*,
            void*
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("GenericObject API is not supported for type {}", str()).do_throw();
    }

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_container(
            hermes::HermesCtrView*
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("GenericObject API is not supported for type {}", str()).do_throw();
    }

    virtual void hermes_serialize(
            hermes::ValueStorageTag,
            hermes::ValueStorage&,
            hermes::SerializationState&
    ) {
        MEMORIA_MAKE_GENERIC_ERROR("Hermes binary serialization is not supported for type {}", str()).do_throw();
    }

    virtual void hermes_check(
            const void* addr,
            hermes::CheckStructureState& state,
            const char* src
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Hermes structure checking is not supported for type {} at {}", str(), src).do_throw();
    }

    virtual void hermes_check_embedded(
            const arena::EmbeddingRelativePtr<void>& ptr,
            hermes::CheckStructureState& state,
            const char* src
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Hermes structure checking is not supported for type {} at {}", str(), src).do_throw();
    }
};




template <typename T>
class TypeReflectionImplBase: public TypeReflection {
public:
    TypeReflectionImplBase() {}

    virtual const std::type_info& type_info() const noexcept override {
        return typeid(T);
    }

    virtual U8String str() const override {
        return TypeNameFactory<T>::name();
    }

    virtual bool hermes_is_simple_layout(
            LWMemHolder* ref_holder,
            void* ptr
    ) const override {
        MEMORIA_MAKE_GENERIC_ERROR("hermes_is_simple_layout() is not implemented for type {}", str()).do_throw();
    }
};

template <typename T>
class TypeCodeTypeReflectionImplBase: public TypeReflectionImplBase<T> {
public:
    TypeCodeTypeReflectionImplBase() {}
};

template <typename T>
class CtrTypeReflectionImpl: public TypeReflectionImplBase<T> {
public:
    virtual Any create_cxx_instance(const hermes::Datatype&) {
        return Any(T{});
    }
};


TypeReflection& get_type_reflection(ShortTypeCode short_type_hash);
TypeReflection& get_type_reflection(const UID256& type_hash);

bool has_type_reflection(ShortTypeCode short_type_hash);
bool has_type_reflection(const UID256& type_hash);

void for_each_type_reflection(std::function<void (const UID256&, TypeReflection&)> fn);
void for_each_type_reflection(std::function<void (const ShortTypeCode&, TypeReflection&)> fn);


void register_type_reflection(TypeReflection& type_reflection);
void register_type_reflection(const UID256& type_code, TypeReflection& type_reflection);
void register_type_reflection_256(TypeReflection& type_reflection);


U8StringView get_datatype_name(U8StringView name);
Any get_cxx_instance(const hermes::Datatype& typedecl);



template <typename T>
void register_ctr_type_reflection()
{
    auto timpl = std::make_shared<CtrTypeReflectionImpl<T>>();
    register_type_reflection_256(*timpl);

    //std::shared_ptr<DataTypeOperations> ops = std::make_shared<DataTypeOperationsImpl<T>>();

    //DataTypeRegistryStore::global().template register_notctr_operations<T>(ops);
    //DataTypeRegistry::local().template register_notctr_operations<T>(ops);
}




namespace arena {
    template <typename T>
    class RelativePtr;
}

namespace detail {

template <typename T>
struct DeepCopyHelper {
    static void deep_copy_to(
            arena::AddrResolver<T>& dst,
            const T* src, size_t size,
            hermes::DeepCopyState& dedup
    ) {
        std::memcpy(dst.get(dedup.arena()), src, size * sizeof(T));
    }
};

template <typename T>
struct DeepCopyHelper<arena::RelativePtr<T>> {
    static void deep_copy_to(
            arena::AddrResolver<arena::RelativePtr<T>>& dst,
            const arena::RelativePtr<T>* src, size_t size,
            hermes::DeepCopyState& dedup
    )
    {
        for (size_t c = 0; c < size; c++)
        {
            if (src[c].is_not_null())
            {
                auto tag = arena::read_type_tag(src[c].get());
                T* ptr = ptr_cast<T>(get_type_reflection(tag).deep_copy_to(src[c].get(), dedup));
                dst.get(dedup.arena())[c] = ptr;
            }
            else {
                dst.get(dedup.arena())[c] = nullptr;
            }
        }
    }
};


template <typename T>
struct DeepCopyHelper<arena::EmbeddingRelativePtr<T>> {
    static void deep_copy_to(
            arena::AddrResolver<arena::EmbeddingRelativePtr<T>>& dst,
            const arena::EmbeddingRelativePtr<T>* src, size_t size,
            hermes::DeepCopyState& dedup
    )
    {
        for (size_t c = 0; c < size; c++)
        {
            if (src[c].is_pointer())
            {
                if (src[c].is_not_null())
                {
                    auto tag = arena::read_type_tag(src[c].get());
                    T* ptr = ptr_cast<T>(get_type_reflection(tag).deep_copy_to(src[c].get(), dedup));
                    dst.get(dedup.arena())[c] = ptr;
                }
                else {
                    dst.get(dedup.arena())[c] = nullptr;
                }
            }
            else {
                dst.get(dedup.arena())[c] = src[c];
            }
        }
    }
};

}

}
