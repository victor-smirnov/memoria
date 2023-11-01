
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
class AbstractHermesTypeReflectionImpl: public TypeCodeTypeReflectionImplBase<T> {
public:

    virtual void hermes_stringify_value(
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,
            std::ostream& out,
            hermes::DumpFormatState& state
    ) const override {
        if (vs_tag == hermes::ValueStorageTag::VS_TAG_ADDRESS) {
            T(ref_holder, ptr.addr).stringify(out, state);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("ValueStorageTag of {} is not supported for type {}", (int64_t)vs_tag, this->str()).do_throw();
        }
    }

    virtual bool hermes_is_simple_layout(
            LWMemHolder* ref_holder,
            void* ptr
    ) const override {
        return T(ref_holder, ptr).is_simple_layout();
    }

    virtual void* deep_copy_to(
            void* ptr,
            hermes::DeepCopyState& dedup) const override
    {
        return T(dedup.mem_holder(), ptr).deep_copy_to(dedup);
    }

    virtual void hermes_check(
            const void* addr,
            hermes::CheckStructureState& state,
            const char*
    ) const override {
        T::check_structure(addr, state);
    }
};



template <typename T>
class HermesTypeReflectionImpl: public AbstractHermesTypeReflectionImpl<T> {
public:

    // Not all types may have short type hash
    // TypeHash<T>
    virtual ShortTypeCode shot_type_hash() const noexcept override {
        return ShortTypeCode::of<T>();
    };


    virtual bool hermes_equals(
            LWMemHolder* left_ptr,
            hermes::ValueStorageTag left_vs_tag,
            hermes::ValueStorage& left,
            LWMemHolder* right_ptr,
            hermes::ValueStorageTag right_vs_tag,
            hermes::ValueStorage& right
    ) const override
    {
        auto right_tag = hermes::get_type_tag(right_vs_tag, right);
        auto left_tag  = hermes::get_type_tag(left_vs_tag,  left);

        if (right_tag == left_tag)
        {
            T left_obj(left_ptr, left.addr);
            T right_obj(right_ptr, right.addr);

            return left_obj == right_obj;
        }

        return false;
    }
};

namespace detail {

template <typename T> struct GenericCtrDispatcher;

template <typename DT>
struct GenericCtrDispatcher<hermes::Array<DT>> {
    static hermes::GenericObjectPtr create_ctr(hermes::HermesCtrView* ctr) {
        return ctr->make_array<DT>().as_object().as_generic_array();
    }
};

template <>
struct GenericCtrDispatcher<hermes::Array<hermes::Object>> {
    static hermes::GenericObjectPtr create_ctr(hermes::HermesCtrView* ctr) {
        return ctr->make_array<hermes::Object>().as_object().as_generic_array();
    }
};

template <>
struct GenericCtrDispatcher<hermes::Map<Varchar, hermes::Object>> {
    static hermes::GenericObjectPtr create_ctr(hermes::HermesCtrView* ctr) {
        return ctr->make_object_map().as_object().as_generic_map();
    }
};

template <typename KeyDT, typename ValueDT>
struct GenericCtrDispatcher<hermes::Map<KeyDT, ValueDT>> {
    static hermes::GenericObjectPtr create_ctr(hermes::HermesCtrView* ctr) {
        return ctr->make_map<KeyDT, ValueDT>().as_object().as_generic_map();
    }
};

}

template <typename T, typename GenericCtrImplT>
class HermesContainerTypeReflectionImpl: public AbstractHermesTypeReflectionImpl<T> {
public:

    // Not all types may have short type hash
    // TypeHash<T>
    virtual ShortTypeCode shot_type_hash() const noexcept override {
        return ShortTypeCode::of<T>();
    };

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_wrapper(
            LWMemHolder* ref_holder,
            void* addr
    ) const override {
        return GenericCtrImplT::make_wrapper(T(ref_holder, addr));
    }

    virtual PoolSharedPtr<hermes::GenericObject> hermes_make_container(
            hermes::HermesCtrView* ctr
    ) const override {
        return detail::GenericCtrDispatcher<T>::create_ctr(ctr);
    }


    virtual bool hermes_equals(
            LWMemHolder* left_ptr,
            hermes::ValueStorageTag left_vs_tag,
            hermes::ValueStorage& left,
            LWMemHolder* right_ptr,
            hermes::ValueStorageTag right_vs_tag,
            hermes::ValueStorage& right
    ) const override
    {
        auto right_tag = hermes::get_type_tag(right_vs_tag, right);
        auto left_tag  = hermes::get_type_tag(left_vs_tag,  left);

        if (right_tag == left_tag)
        {
            T left_obj(left_ptr, left.addr);
            T right_obj(right_ptr, right.addr);

            return left_obj == right_obj;
        }

        return false;
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

    __attribute__((noreturn)) static U8String convert_to(DTTViewType<DT>) {
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

    __attribute__((noreturn)) static hermes::Object convert_from(U8StringView) {
        MEMORIA_MAKE_GENERIC_ERROR("No 'from string' converter is defined for datatype {}", TypeNameFactory<DT>::name()).do_throw();
    }
};

template <typename DT>
struct FromStringHelper<DT, true> {
    static bool is_convertible() noexcept {
        return true;
    }

    static hermes::Object convert_from(U8StringView view) {
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
        mapping[ShortTypeCode::of<ToDT>().u64()] = std::make_unique<DatatypeConverter<FromDT, ToDT>>();
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

template<typename T, typename List>
struct ContainsInH;

template<typename T, typename... Tail>
struct ContainsInH<T, TL<T, Tail...>>: HasValue<bool, true> {};

template<typename T, typename Head, typename... Tail>
struct ContainsInH<T, TL<Head, Tail...>>: HasValue<bool, ContainsInH<T, TL<Tail...>>::Value> {};

template<typename T>
struct ContainsInH<T, TL<>>: HasValue<bool, false> {};

template <typename DT>
using NumericTypeSelector = IfThenElse<ContainsInH<DT, AllNumericDatatypes>::Value, NumericDatatype, EmptyType>;

template <typename LeftDT, typename RightDT>
using NumericTypeSelector2 = IfThenElse<
    ContainsInH<LeftDT, AllNumericDatatypes>::Value &&
    ContainsInH<RightDT, AllNumericDatatypes>::Value,
    NumericDatatype, EmptyType
>;



template <typename T, typename DT, bool Selector>
struct SameDatatypeComparatorSelector;

template <typename T, typename DT>
struct SameDatatypeComparatorSelector<T, DT, true> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<DT>().u64()] = [](
                const DTTViewType<DT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return DatatypeComparator<DT, NumericTypeSelector<DT>>::compare(
                left_view,
                right_object.template view_unchecked<DT>()
            );
        };
    }
};

template <typename T, typename DT>
struct SameDatatypeComparatorSelector<T, DT, false> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<DT>().u64()] = [](
                const DTTViewType<DT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return CrossDatatypeComparator<DT, DT>::compare(
                        left_view,
                        right_object.template view_unchecked<DT>()
            );
        };
    }
};


template <typename T, typename LeftDT, typename RightDT, bool Selector>
struct CrossDatatypeComparatorSelector;

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeComparatorSelector<T, LeftDT, RightDT, true> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<RightDT>().u64()] = [](
                const DTTViewType<LeftDT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return CrossDatatypeComparator<LeftDT, RightDT, NumericTypeSelector<LeftDT>>::
                    compare(left_view, right_object.template view_unchecked<RightDT>());
        };
    }
};

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeComparatorSelector<T, LeftDT, RightDT, false> {
    template <typename MapView>
    static void build_mapping(MapView&) noexcept {}
};


// All Datatypes in this list comparable (either comparator type is defined).
template <typename T, typename DT, typename List>
struct ComparatorsMapBuilder;

// All Datatypes in this specialization are different.
// Same-datatype case is handled separately.
template <typename T, typename LeftDT, typename RightDT, typename... Tail>
struct ComparatorsMapBuilder<T, LeftDT, TL<RightDT, Tail...>> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        CrossDatatypeComparatorSelector<
                T, LeftDT, RightDT,
                IsComplete<
                    CrossDatatypeComparator<LeftDT, RightDT, NumericTypeSelector2<LeftDT, RightDT>>
                >::value
        >::build_mapping(map);
        ComparatorsMapBuilder<T, LeftDT, TL<Tail...>>::build_mapping(map);
    }
};

// Same-datatype case.
template <typename T, typename DT, typename... Tail>
struct ComparatorsMapBuilder<T, DT, TL<DT, Tail...>> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        // At least one of comparators is defined here.
        // Pick up Same-type comparator if it's defined.
        // Otherwise pick up cross-type one.
        SameDatatypeComparatorSelector<
                T,
                DT,
                IsComplete<DatatypeComparator<DT, NumericTypeSelector<DT>>>::value
        >::build_mapping(map);
        ComparatorsMapBuilder<T, DT, TL<Tail...>>::build_mapping(map);
    }
};

template <typename T, typename DT>
struct ComparatorsMapBuilder<T, DT, TL<>> {
    template <typename MapView>
    static void build_mapping(MapView&) noexcept {}
};


template <typename T, typename DT, bool Selector>
struct SameDatatypeEqualityComparatorSelector;

template <typename T, typename DT>
struct SameDatatypeEqualityComparatorSelector<T, DT, true> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<DT>().u64()] = [](
                const DTTViewType<DT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return DatatypeEqualityComparator<DT, NumericTypeSelector<DT>>::equals(
                left_view,
                right_object.template view_unchecked<DT>()
            );
        };
    }
};

template <typename T, typename DT>
struct SameDatatypeEqualityComparatorSelector<T, DT, false> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<DT>().u64()] = [](
                const DTTViewType<DT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return CrossDatatypeEqualityComparator<DT, DT>::equals(
                left_view,
                right_object.template view_unchecked<DT>()
            );
        };
    }
};


template <typename T, typename LeftDT, typename RightDT, bool Selector>
struct CrossDatatypeEqualityComparatorSelector;

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeEqualityComparatorSelector<T, LeftDT, RightDT, true> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        map[ShortTypeCode::of<RightDT>().u64()] = [](
                const DTTViewType<LeftDT>& left_view,
                LWMemHolder* right_ptr,
                hermes::ValueStorageTag right_vs_tag, hermes::ValueStorage& right
        ) {
            T right_object(right_ptr, right_vs_tag, right);
            return CrossDatatypeEqualityComparator<LeftDT, RightDT, NumericTypeSelector<LeftDT>>::
                equals(left_view, right_object.template view_unchecked<RightDT>());
        };
    }
};

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeEqualityComparatorSelector<T, LeftDT, RightDT, false> {
    template <typename MapView>
    static void build_mapping(MapView&) noexcept {}
};



// Equality comparators are subset of Comparators.
// If there is an equality comparator for a type or type pair,
// it will be used. Otherwise, generic comparator will be used.

// All Datatypes in this list are equality comparable (either comparator type is defined).
template <typename T, typename DT, typename List>
struct EqualityComparatorsMapBuilder;

// All Datatypes in this specialization are different.
// Same-datatype case is handled separately.
template <typename T, typename LeftDT, typename RightDT, typename... Tail>
struct EqualityComparatorsMapBuilder<T, LeftDT, TL<RightDT, Tail...>> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        CrossDatatypeEqualityComparatorSelector<
                T, LeftDT, RightDT,
                IsComplete<
                    CrossDatatypeEqualityComparator<LeftDT, RightDT, NumericTypeSelector2<LeftDT, RightDT>>
                >::value
        >::build_mapping(map);
        EqualityComparatorsMapBuilder<T, LeftDT, TL<Tail...>>::build_mapping(map);
    }
};


// Same-datatype case.
template <typename T, typename DT, typename... Tail>
struct EqualityComparatorsMapBuilder<T, DT, TL<DT, Tail...>> {
    template <typename MapView>
    static void build_mapping(MapView& map) noexcept {
        // At least one of comparators is defined here.
        // Pick up Same-type comparator if it's defined.
        // Otherwise pick up cross-type one.
        SameDatatypeEqualityComparatorSelector<
                T,
                DT,
                IsComplete<DatatypeEqualityComparator<DT, NumericTypeSelector<DT>>>::value
        >::build_mapping(map);
        EqualityComparatorsMapBuilder<T, DT, TL<Tail...>>::build_mapping(map);
    }
};

template <typename T, typename DT>
struct EqualityComparatorsMapBuilder<T, DT, TL<>> {
    template <typename MapView>
    static void build_mapping(MapView&) noexcept {}
};


template <typename List> struct ComparableTypesH;

template <typename Head, typename... Tail>
struct ComparableTypesH<TL<Head, Tail...>> {
    using Type = MergeLists<IfThenElse<
        IsComplete<DatatypeComparator<Head, NumericTypeSelector<Head>>>::value ||
        IsComplete<CrossDatatypeComparator<Head, Head, NumericTypeSelector<Head>>>::value,
        Head,
        TL<>
    >, typename ComparableTypesH<TL<Tail...>>::Type>;
};

template <>
struct ComparableTypesH<TL<>> {
    using Type = TL<>;
};

template <typename List> struct EqualityComparableTypesH;

template <typename Head, typename... Tail>
struct EqualityComparableTypesH<TL<Head, Tail...>> {
    using Type = MergeLists<IfThenElse<
        IsComplete<DatatypeEqualityComparator<Head, NumericTypeSelector<Head>>>::value ||
        IsComplete<CrossDatatypeEqualityComparator<Head, Head, NumericTypeSelector<Head>>>::value,
        Head,
        TL<>
    >, typename EqualityComparableTypesH<TL<Tail...>>::Type>;
};

template <>
struct EqualityComparableTypesH<TL<>> {
    using Type = TL<>;
};


using AllComparableDatatypes = typename ComparableTypesH<AllHermesDatatypes>::Type;
using AllEqualityComparableDatatypes = typename EqualityComparableTypesH<AllHermesDatatypes>::Type;


template <uint64_t LeftCode, typename List, int32_t Idx = 0>
struct LeftIndexOf;

template <uint64_t TypeCode, typename Head, typename... Tail, int32_t Idx>
struct LeftIndexOf<TypeCode, TL<Head, Tail...>, Idx> {
    static constexpr int32_t ObjectView = IfThenElse<
            ShortTypeCode::of<Head>().u64() == TypeCode,
            IntValue<Idx>,
            IntValue<LeftIndexOf<TypeCode, TL<Tail...>, Idx + 1>::ObjectView>
    >::Value;
};

template <uint64_t TypeCode, int32_t Idx>
struct LeftIndexOf<TypeCode, TL<>, Idx> {
    static constexpr int32_t ObjectView = Idx;
};

template <typename List>
struct RightIndexOf;

template <typename Head, typename... Tail>
struct RightIndexOf<TL<Head, Tail...>> {
    static int32_t find(ShortTypeCode type_code, int32_t idx = 0) noexcept {
        if (ShortTypeCode::of<Head>() == type_code) {
            return idx;
        }
        else {
            return RightIndexOf<TL<Tail...>>::find(type_code, idx + 1);
        }
    }
};

template <>
struct RightIndexOf<TL<>> {
    static int32_t find(ShortTypeCode type_code, int32_t idx = 0) noexcept {
        return idx;
    }
};


template <typename ViewT>
using DTEqualityComparator = std::function<bool (const ViewT&, LWMemHolder*, hermes::ValueStorageTag, hermes::ValueStorage&)>;

template <typename ViewT>
using DTComparator = std::function<int32_t (const ViewT&, LWMemHolder* right_ptr, hermes::ValueStorageTag, hermes::ValueStorage&)>;


template <typename TV, bool Embeddable>
struct TVEmbedHelper {
    static bool embed(uint64_t type_code, arena::ERelativePtr& dst, const hermes::TaggedValue& src)
    {
        dst.embed(type_code, src.get_unchecked<TV>());
        return true;
    }
};

template <typename TV>
struct TVEmbedHelper<TV, false> {
    static bool embed(uint64_t, arena::ERelativePtr&, const hermes::TaggedValue&) {
        return false;
    }
};



}

template <typename DT>
class HermesTypeReflectionDatatypeImpl: public TypeCodeTypeReflectionImplBase<DT> {

    using Base = TypeCodeTypeReflectionImplBase<DT>;
    using T = hermes::ObjectView;

    ska::flat_hash_map<uint64_t, std::unique_ptr<IDatatypeConverter>> converters_;
    ska::flat_hash_map<uint64_t, detail::DTComparator<DTTViewType<DT>>> comparators_;
    ska::flat_hash_map<uint64_t, detail::DTEqualityComparator<DTTViewType<DT>>> equality_comparators_;

    static constexpr int32_t LeftTypeIdx = detail::LeftIndexOf<ShortTypeCode::of<DT>().u64(), detail::AllComparableDatatypes>::ObjectView;
    static constexpr int32_t Comparables = ListSize<detail::AllComparableDatatypes>;

public:
    using Base::str;

    // Not all types may have short type hash
    // TypeHash<T>
    virtual ShortTypeCode shot_type_hash() const noexcept override {
        return ShortTypeCode::of<DT>();
    };

    HermesTypeReflectionDatatypeImpl()
    {
        detail::DTConverterListBuilder<DT, AllHermesDatatypes>::add_converters(converters_);
        detail::ComparatorsMapBuilder<T, DT, detail::AllComparableDatatypes>::build_mapping(comparators_);
        detail::EqualityComparatorsMapBuilder<T, DT, detail::AllEqualityComparableDatatypes>::build_mapping(equality_comparators_);
    }

    virtual void hermes_stringify_value(
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr,
            std::ostream& out,
            hermes::DumpFormatState& state
    ) const override {
        T(ref_holder, vs_tag, ptr).stringify_dt<DT>(out, state);
    }

    virtual bool is_convertible_to(ShortTypeCode type_hash) const override {
        return converters_.find(type_hash.u64()) != converters_.end();
    }

    virtual bool is_convertible_to_plain_string() const override {
        return detail::ToStringHelper<DT>::is_convertible();
    }

    virtual bool is_convertible_from_plain_string() const override {
        return detail::FromStringHelper<DT>::is_convertible();
    }

    virtual hermes::Object datatype_convert_to(
            LWMemHolder* ref_holder,
            ShortTypeCode type_hash,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr
    ) const override
    {
        auto ii = converters_.find(type_hash.u64());
        if (ii != converters_.end())
        {
            T data_object(ref_holder, vs_tag, ptr);
            auto view = data_object.view_unchecked<DT>();
            return ii->second.get()->convert(&view);
        }
        else {
            U8String to_type = get_type_reflection(type_hash).str();
            MEMORIA_MAKE_GENERIC_ERROR("No type converter is defined from {} to datatype {}", TypeNameFactory<DT>::name(), to_type).do_throw();
        }
    }

    virtual hermes::Object datatype_convert_from_plain_string(U8StringView str) const override {
        return detail::FromStringHelper<DT>::convert_from(str);
    }

    virtual U8String convert_to_plain_string(
            LWMemHolder* ref_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& ptr
    ) const override
    {
        T data_object(ref_holder, vs_tag, ptr);
        return detail::ToStringHelper<DT>::convert_to(data_object.view_unchecked<DT>());
    }

    virtual bool hermes_comparable_with(ShortTypeCode tag) const override
    {
        if (LeftTypeIdx < Comparables) {
            int32_t right_idx = detail::RightIndexOf<detail::AllComparableDatatypes>::find(tag);
            if (right_idx < Comparables) {
                return true;
            }
        }
        return false;
    }


    virtual bool hermes_equals_comparable_with(ShortTypeCode tag) const override
    {
        return equality_comparators_.find(tag.u64()) != equality_comparators_.end();
    }

    virtual bool hermes_comparable() const override {
        return LeftTypeIdx < Comparables;
    }

    virtual int32_t hermes_compare(
            LWMemHolder* left_ptr,
            hermes::ValueStorageTag left_vs_tag,
            hermes::ValueStorage& left,
            LWMemHolder* right_ptr,
            hermes::ValueStorageTag right_vs_tag,
            hermes::ValueStorage& right
    ) const override
    {
        auto tag = hermes::get_type_tag(right_vs_tag, right);
        if (tag.u64())
        {            
            auto ii = comparators_.find(tag.u64());
            if (ii != comparators_.end())
            {
                T left_data_object(left_ptr, left_vs_tag, left);
                return ii->second(left_data_object.view_unchecked<DT>(), right_ptr, right_vs_tag, right);
            }
            else {
                int32_t right_type_idx = detail::RightIndexOf<detail::AllComparableDatatypes>::find(tag);
                if (right_type_idx < Comparables) {
                    return right_type_idx - LeftTypeIdx;
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Types {} and {} are not comparable", str(), get_type_reflection(tag).str()).do_throw();
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Right argument is nullptr").do_throw();
        }
    }

    virtual bool hermes_equals(
            LWMemHolder* left_ptr,
            hermes::ValueStorageTag left_vs_tag,
            hermes::ValueStorage& left,
            LWMemHolder* right_ptr,
            hermes::ValueStorageTag right_vs_tag,
            hermes::ValueStorage& right
    ) const override
    {
        auto tag = hermes::get_type_tag(right_vs_tag, right);
        if (tag.u64()) {
            auto ii = equality_comparators_.find(tag.u64());
            if (ii != equality_comparators_.end())
            {
                T left_data_object(left_ptr, left_vs_tag, left);
                return ii->second(left_data_object.view_unchecked<DT>(), right_ptr, right_vs_tag, right);
            }
            else {
                return false;
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Right argument is nullptr").do_throw();
        }
    }

    hermes::Object import_value(
            LWMemHolder* mem_holder,
            hermes::ValueStorageTag vs_tag,
            hermes::ValueStorage& storage
    ) const override
    {
        auto view = storage.get_view<DT>(vs_tag, mem_holder);
        auto ctr = hermes::HermesCtr(mem_holder);
        return ctr.new_dataobject<DT>(view);
    }

    virtual bool hermes_is_ptr_embeddable() const override
    {
        bool eptr_fits = arena::ERelativePtr::dt_fits_in<DT>();
        return eptr_fits;
    }

    virtual bool hermes_embed(arena::ERelativePtr& dst, const hermes::TaggedValue& src) const override
    {
        using DV = DTView<DT>;
        constexpr bool eptr_fits = arena::ERelativePtr::dt_fits_in<DT>();
        return detail::TVEmbedHelper<DV, eptr_fits>::embed(TypeHashV<DT>, dst, src);
    }

    virtual bool hermes_is_simple_layout(
            LWMemHolder* mem_holder,
            void* ptr
    ) const override {
        return T(mem_holder, ptr).is_simple_layout_dt<DT>();
    }

    virtual void* deep_copy_to(
            void* ptr,
            hermes::DeepCopyState& dedup) const override
    {
        return T(dedup.mem_holder(), ptr).deep_copy_to_dt<DT>(dedup);
    }

    virtual void hermes_check(
            const void* addr,
            hermes::CheckStructureState& state,
            const char* src
    ) const override {
        T::check_dataobject<DT>(addr, state, src);
    }

    virtual void hermes_check_embedded(
            const arena::EmbeddingRelativePtr<void>& ptr,
            hermes::CheckStructureState& state,
            const char* src
    ) const override {
         T::check_dataobject<DT>(ptr, state, src);
    }
};

}
