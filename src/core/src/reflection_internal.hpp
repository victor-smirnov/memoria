
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

#include <memoria/core/hermes/hermes.hpp>

#include <memoria/core/reflection/reflection.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include "datatype_convertion.hpp"

namespace memoria {

template <typename T>
class HermesTypeReflectionImpl: public TypehashTypeReflectionImplBase<T> {
public:
    virtual void hermes_stringify_value(
            void* ptr,
            hermes::DocView* doc,
            ViewPtrHolder* ref_holder,

            std::ostream& out,
            hermes::DumpFormatState& state,
            hermes::DumpState& dump_state
    ) const {
        T(ptr, doc, ref_holder).stringify(out, state, dump_state);
    }

    virtual bool hermes_is_simple_layout(
            void* ptr,
            hermes::DocView* doc,
            ViewPtrHolder* ref_holder
    ) const {
        return T(ptr, doc, ref_holder).is_simple_layout();
    }

    virtual void* deep_copy_to(
            arena::ArenaAllocator& arena,
            void* ptr,
            void* owner_view,
            ViewPtrHolder* ref_holder,
            DeepCopyDeduplicator& dedup) const
    {
        hermes::DocView* doc = reinterpret_cast<hermes::DocView*>(owner_view);
        return T(ptr, doc, ref_holder).deep_copy_to(arena, dedup);
    }
};

namespace detail {

template <typename DT, bool IsTComplete = IsComplete<ToPlainStringConverter<DT>>::value>
struct ToStringHelper;

template <typename DT, bool IsTComplete = IsComplete<FromPlainStringConverter<DT>>::value>
struct FromStringHelper;

template <typename DT>
struct ToStringHelper<DT, false> {
    static bool is_convertible() noexcept {
        return false;
    }

    static U8String convert_to(DTTViewType<DT>) {
        MEMORIA_MAKE_GENERIC_ERROR("No 'to string' converter is defined for datatype {}", TypeNameFactory<DT>::name()).do_throw();
    }
};


template <typename DT>
struct ToStringHelper<DT, true> {
    static bool is_convertible() noexcept {
        return true;
    }

    static U8String convert_to(DTTViewType<DT> view) {
        return ToPlainStringConverter<DT>::to_string(view);
    }
};

template <typename DT>
struct FromStringHelper<DT, false> {
    static bool is_convertible() noexcept {
        return false;
    }

    static PoolSharedPtr<hermes::DocView> convert_from(U8StringView) {
        MEMORIA_MAKE_GENERIC_ERROR("No 'from string' converter is defined for datatype {}", TypeNameFactory<DT>::name()).do_throw();
    }
};

template <typename DT>
struct FromStringHelper<DT, true> {
    static bool is_convertible() noexcept {
        return true;
    }

    static PoolSharedPtr<hermes::DocView> convert_from(U8StringView view) {
        return FromPlainStringConverter<DT>::from_string(view);
    }
};


template <typename FromDT, typename ToDT, bool IsTComplete = IsComplete<DatatypeConverter<FromDT, ToDT>>::value>
struct TypeCvtBuilderHelper;


template <typename FromDT, typename ToDT>
struct TypeCvtBuilderHelper<FromDT, ToDT, false> {
    template <typename Mapping>
    static void add_converter (Mapping&) {}
};


template <typename FromDT, typename ToDT>
struct TypeCvtBuilderHelper<FromDT, ToDT, true> {
    template <typename Mapping>
    static void add_converter (Mapping& mapping) {
        mapping[TypeHashV<ToDT>] = std::make_unique<DatatypeConverter<FromDT, ToDT>>();
    }
};


template <typename FromDT, typename FromList>
struct DTConverterListBuilder;

template <typename FromDT, typename ToDT, typename... Tail>
struct DTConverterListBuilder<FromDT, TL<ToDT, Tail...>> {

    template <typename Mapping>
    static void add_converters(Mapping& mapping) {
        TypeCvtBuilderHelper<FromDT, ToDT>::add_converter(mapping);
        DTConverterListBuilder<FromDT, TL<Tail...>>::add_converters(mapping);
    }
};

template <typename FromDT>
struct DTConverterListBuilder<FromDT, TL<>> {
    template <typename Mapping>
    static void add_converters(Mapping&) {
    }
};


}


using AllHermesDatatypes = TL<
    Varchar, BigInt, UBigInt, Boolean, Double, Real, TinyInt, UTinyInt, SmallInt, USmallInt, Integer, UInteger
>;

template <typename T, typename DT>
class HermesTypeReflectionDatatypeImpl: public HermesTypeReflectionImpl<T> {

    ska::flat_hash_map<uint64_t, std::unique_ptr<IDatatypeConverter>> converters_;

public:
    HermesTypeReflectionDatatypeImpl() {
        detail::DTConverterListBuilder<DT, AllHermesDatatypes>::add_converters(converters_);
    }

    virtual bool is_convertible_to(uint64_t type_hash) const {
        return converters_.find(type_hash) != converters_.end();
    }

    virtual bool is_convertible_to_plain_string() const {
        return detail::ToStringHelper<DT>::is_convertible();
    }

    virtual bool is_convertible_from_plain_string() const {
        return detail::FromStringHelper<DT>::is_convertible();
    }

    virtual PoolSharedPtr<hermes::DocView> datatype_convert_to(
            uint64_t type_hash, void* ptr,
            hermes::DocView* doc,
            ViewPtrHolder* ref_holder
    ) const
    {
        auto ii = converters_.find(type_hash);
        if (ii != converters_.end())
        {
            T data_object(ptr, doc, ref_holder);
            auto view = data_object.view();
            return ii->second.get()->convert(&view);
        }
        else {
            U8String to_type = get_type_reflection(type_hash).str();
            MEMORIA_MAKE_GENERIC_ERROR("No type converter is defined from {} to datatype {}", TypeNameFactory<DT>::name(), to_type).do_throw();
        }
    }

    virtual PoolSharedPtr<hermes::DocView> datatype_convert_from_plain_string(U8StringView str) const {
        return detail::FromStringHelper<DT>::convert_from(str);
    }

    virtual U8String convert_to_plain_string(void* ptr,
                                             hermes::DocView* doc,
                                             ViewPtrHolder* ref_holder) const
    {
        T data_object(ptr, doc, ref_holder);
        return detail::ToStringHelper<DT>::convert_to(data_object.view());
    }

};

}
