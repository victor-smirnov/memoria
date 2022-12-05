
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <typeinfo>
#include <functional>


namespace memoria {

U8String normalize_type_declaration(U8StringView type_decl);
UID256 compute_full_type_hash(U8StringView type_decl);
UID256 compute_full_type_hash_normalized(U8StringView type_decl);

struct IDatatypeConverter {
    virtual ~IDatatypeConverter() noexcept;

    virtual hermes::ObjectPtr convert(const void* view) const = 0;
};


class DeepCopyDeduplicator;

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
    virtual ShortTypeCode shot_type_hash() const noexcept = 0;

    virtual void hermes_stringify_value(
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,
            ViewPtrHolder* ref_holder,

            std::ostream& out,
            hermes::DumpFormatState& state
    ) const = 0;

    virtual bool hermes_is_simple_layout(
            void* ptr,
            ViewPtrHolder* ref_holder
    ) const = 0;

    template <typename T>
    T* deep_copy(arena::ArenaAllocator& arena,
                 T* src,
                 ViewPtrHolder* ptr_holder,
                 DeepCopyDeduplicator& dedup) const {
        return reinterpret_cast<T*>(
            deep_copy_to(arena, src, ptr_holder, dedup)
        );
    }

    virtual void* deep_copy_to(
            arena::ArenaAllocator&,
            void*,
            ViewPtrHolder*,
            DeepCopyDeduplicator&
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

    virtual hermes::ObjectPtr datatype_convert_to(
            ShortTypeCode target_tag,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,
            ViewPtrHolder* ref_holder
    ) const ;

    virtual hermes::ObjectPtr datatype_convert_from_plain_string(U8StringView str) const;

    virtual U8String convert_to_plain_string(
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,
            ViewPtrHolder* ref_holder
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

    virtual int32_t hermes_compare(
            hermes::ValueStorageTag,
            hermes::ValueStorage&, ViewPtrHolder*,
            hermes::ValueStorageTag,
            hermes::ValueStorage&, ViewPtrHolder*
    ) const
    {
        MEMORIA_MAKE_GENERIC_ERROR("Objects of the same type {} are not Hermes-comparable", str()).do_throw();
    }

    virtual bool hermes_equals(
            hermes::ValueStorageTag,
            hermes::ValueStorage&, ViewPtrHolder*,
            hermes::ValueStorageTag,
            hermes::ValueStorage&, ViewPtrHolder*
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("Objects of type {} are not Hermes-equals-comparable", str()).do_throw();
    }

    virtual hermes::ObjectPtr import_value(
            hermes::ValueStorageTag, hermes::ValueStorage&,
            ViewPtrHolder*
    ) const;

    virtual bool hermes_is_ptr_embeddable() const {
        return false;
    }

    virtual bool hermes_embed(arena::ERelativePtr& dst, const hermes::TaggedValue&) const {
        return false;
    }

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_wrapper(
            void*,
            ViewPtrHolder*
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("GenericObject API is not supported for type {}", str()).do_throw();
    }

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_container(
            hermes::HermesCtr*
    ) const {
        MEMORIA_MAKE_GENERIC_ERROR("GenericObject API is not supported for type {}", str()).do_throw();
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
            void* ptr,
            ViewPtrHolder* ref_holder
    ) const override {
        MEMORIA_MAKE_GENERIC_ERROR("hermes_is_simple_layout() is not implemented for type {}", str()).do_throw();
    }
};

template <typename T>
class TypeCodeTypeReflectionImplBase: public TypeReflectionImplBase<T> {
public:
    TypeCodeTypeReflectionImplBase() {}

    // Not all types may have short type hash
    // TypeHash<T>
    virtual ShortTypeCode shot_type_hash() const noexcept override {
        return ShortTypeCode::of<T>();
    };
};



TypeReflection& get_type_reflection(ShortTypeCode short_type_hash);
TypeReflection& get_type_reflection(const UID256& type_hash);

bool has_type_reflection(ShortTypeCode short_type_hash);
bool has_type_reflection(const UID256& type_hash);

void for_each_type_reflection(std::function<void (const UID256&, TypeReflection&)> fn);
void for_each_type_reflection(std::function<void (const ShortTypeCode&, TypeReflection&)> fn);


void register_type_reflection(TypeReflection& type_reflection);
void register_type_reflection(const UID256& type_code, TypeReflection& type_reflection);

U8StringView get_datatype_name(U8StringView name);


class DeepCopyDeduplicator {
    ska::flat_hash_map<const void*, arena::AddrResolver<void>> addr_map_;
public:

    template <typename T>
    T* resolve(arena::ArenaAllocator& arena, const T* src) noexcept
    {
        auto ii = addr_map_.find(src);
        if (ii != addr_map_.end()) {
            return reinterpret_cast<T*>(ii->second.get(arena));
        }
        return nullptr;
    }

    template <typename T>
    void map(arena::ArenaAllocator& arena, const T* src, T* dst) {
        addr_map_[src] = arena.get_resolver_for(static_cast<void*>(dst));
    }
};

namespace arena {
    template <typename T>
    class RelativePtr;
}

namespace detail {

template <typename T>
struct DeepCopyHelper {
    static void deep_copy_to(
            arena::ArenaAllocator& arena,
            arena::AddrResolver<T>& dst,
            const T* src, size_t size,
            ViewPtrHolder* ref_holder,
            DeepCopyDeduplicator& dedup
    ) {
        std::memcpy(dst.get(arena), src, size * sizeof(T));
    }
};

template <typename T>
struct DeepCopyHelper<arena::RelativePtr<T>> {
    static void deep_copy_to(
            arena::ArenaAllocator& arena,
            arena::AddrResolver<arena::RelativePtr<T>>& dst,
            const arena::RelativePtr<T>* src, size_t size,
            ViewPtrHolder* ref_holder,
            DeepCopyDeduplicator& dedup
    )
    {
        for (size_t c = 0; c < size; c++)
        {
            if (src[c].is_not_null())
            {
                auto tag = arena::read_type_tag(src[c].get());
                T* ptr = ptr_cast<T>(get_type_reflection(tag).deep_copy_to(arena, src[c].get(), ref_holder, dedup));
                dst.get(arena)[c] = ptr;
            }
            else {
                dst.get(arena)[c] = nullptr;
            }
        }
    }
};

}


}
