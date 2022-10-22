
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
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<DT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            T right_object(right, right_doc, right_ptr);
            return DatatypeComparator<DT, NumericTypeSelector<DT>>::compare(left_object.view(), right_object.view());
        };
    }
};

template <typename T, typename DT>
struct SameDatatypeComparatorSelector<T, DT, false> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<DT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            T right_object(right, right_doc, right_ptr);
            return CrossDatatypeComparator<DT, DT>::compare(left_object.view(), right_object.view());
        };
    }
};


template <typename T, typename LeftDT, typename RightDT, bool Selector>
struct CrossDatatypeComparatorSelector;

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeComparatorSelector<T, LeftDT, RightDT, true> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<RightDT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            hermes::DataObject<RightDT> right_object(right, right_doc, right_ptr);
            return CrossDatatypeComparator<LeftDT, RightDT, NumericTypeSelector<LeftDT>>::compare(left_object.view(), right_object.view());
        };
    }
};

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeComparatorSelector<T, LeftDT, RightDT, false> {
    template <typename Map>
    static void build_mapping(Map&) noexcept {}
};


// All Datatypes in this list comparable (either comparator type is defined).
template <typename T, typename DT, typename List>
struct ComparatorsMapBuilder;

// All Datatypes in this specialization are different.
// Same-datatype case is handled separately.
template <typename T, typename LeftDT, typename RightDT, typename... Tail>
struct ComparatorsMapBuilder<T, LeftDT, TL<RightDT, Tail...>> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
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
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
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
    template <typename Map>
    static void build_mapping(Map&) noexcept {}
};


template <typename T, typename DT, bool Selector>
struct SameDatatypeEqualityComparatorSelector;

template <typename T, typename DT>
struct SameDatatypeEqualityComparatorSelector<T, DT, true> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<DT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            T right_object(right, right_doc, right_ptr);
               return DatatypeEqualityComparator<DT, NumericTypeSelector<DT>>::equals(left_object.view(), right_object.view());
        };
    }
};

template <typename T, typename DT>
struct SameDatatypeEqualityComparatorSelector<T, DT, false> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<DT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            T right_object(right, right_doc, right_ptr);
            return CrossDatatypeEqualityComparator<DT, DT>::equals(left_object.view(), right_object.view());
        };
    }
};


template <typename T, typename LeftDT, typename RightDT, bool Selector>
struct CrossDatatypeEqualityComparatorSelector;

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeEqualityComparatorSelector<T, LeftDT, RightDT, true> {
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
        map[TypeHashV<RightDT>] = [](const T& left_object, void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) {
            hermes::DataObject<RightDT> right_object(right, right_doc, right_ptr);
            return CrossDatatypeEqualityComparator<LeftDT, RightDT, NumericTypeSelector<LeftDT>>::equals(left_object.view(), right_object.view());
        };
    }
};

template <typename T, typename LeftDT, typename RightDT>
struct CrossDatatypeEqualityComparatorSelector<T, LeftDT, RightDT, false> {
    template <typename Map>
    static void build_mapping(Map&) noexcept {}
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
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
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
    template <typename Map>
    static void build_mapping(Map& map) noexcept {
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
    template <typename Map>
    static void build_mapping(Map&) noexcept {}
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

template <uint64_t HashCode, typename Head, typename... Tail, int32_t Idx>
struct LeftIndexOf<HashCode, TL<Head, Tail...>, Idx> {
    static constexpr int32_t Value = IfThenElse<
            TypeHashV<Head> == HashCode,
            IntValue<Idx>,
            IntValue<LeftIndexOf<HashCode, TL<Tail...>, Idx + 1>::Value>
    >::Value;
};

template <uint64_t HashCode, int32_t Idx>
struct LeftIndexOf<HashCode, TL<>, Idx> {
    static constexpr int32_t Value = Idx;
};

template <typename List>
struct RightIndexOf;

template <typename Head, typename... Tail>
struct RightIndexOf<TL<Head, Tail...>> {
    static int32_t find(uint64_t hash_code, int32_t idx = 0) noexcept {
        if (TypeHashV<Head> == hash_code) {
            return idx;
        }
        else {
            return RightIndexOf<TL<Tail...>>::find(hash_code, idx + 1);
        }
    }
};

template <>
struct RightIndexOf<TL<>> {
    static int32_t find(uint64_t hash_code, int32_t idx = 0) noexcept {
        return idx;
    }
};


template <typename ViewT>
using DTEqualityComparator = std::function<bool (const ViewT&, void*, hermes::DocView*, ViewPtrHolder*)>;

template <typename ViewT>
using DTComparator = std::function<int32_t (const ViewT&, void*, hermes::DocView*, ViewPtrHolder*)>;


}

template <typename T, typename DT>
class HermesTypeReflectionDatatypeImpl: public HermesTypeReflectionImpl<T> {

    using Base = HermesTypeReflectionImpl<T>;

    ska::flat_hash_map<uint64_t, std::unique_ptr<IDatatypeConverter>> converters_;
    ska::flat_hash_map<uint64_t, detail::DTComparator<T>> comparators_;
    ska::flat_hash_map<uint64_t, detail::DTEqualityComparator<T>> equality_comparators_;

    static constexpr int32_t LeftTypeIdx = detail::LeftIndexOf<TypeHashV<DT>, detail::AllComparableDatatypes>::Value;
    static constexpr int32_t Comparables = ListSize<detail::AllComparableDatatypes>;

public:
    using Base::str;

    HermesTypeReflectionDatatypeImpl() {
        detail::DTConverterListBuilder<DT, AllHermesDatatypes>::add_converters(converters_);
        detail::ComparatorsMapBuilder<T, DT, detail::AllComparableDatatypes>::build_mapping(comparators_);
        detail::EqualityComparatorsMapBuilder<T, DT, detail::AllEqualityComparableDatatypes>::build_mapping(equality_comparators_);
    }

    virtual bool is_convertible_to(uint64_t type_hash) const override {
        return converters_.find(type_hash) != converters_.end();
    }

    virtual bool is_convertible_to_plain_string() const override {
        return detail::ToStringHelper<DT>::is_convertible();
    }

    virtual bool is_convertible_from_plain_string() const override {
        return detail::FromStringHelper<DT>::is_convertible();
    }

    virtual PoolSharedPtr<hermes::DocView> datatype_convert_to(
            uint64_t type_hash, void* ptr,
            hermes::DocView* doc,
            ViewPtrHolder* ref_holder
    ) const override
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

    virtual PoolSharedPtr<hermes::DocView> datatype_convert_from_plain_string(U8StringView str) const override {
        return detail::FromStringHelper<DT>::convert_from(str);
    }

    virtual U8String convert_to_plain_string(void* ptr,
                                             hermes::DocView* doc,
                                             ViewPtrHolder* ref_holder) const override
    {
        T data_object(ptr, doc, ref_holder);
        return detail::ToStringHelper<DT>::convert_to(data_object.view());
    }

    virtual bool hermes_comparable_with(uint64_t tag) const override
    {
        if (LeftTypeIdx < Comparables) {
            int32_t right_idx = detail::RightIndexOf<detail::AllComparableDatatypes>::find(tag);
            if (right_idx < Comparables) {
                return true;
            }
        }
        return false;
    }


    virtual bool hermes_equals_comparable_with(uint64_t tag) const override
    {
        return equality_comparators_.find(tag) != equality_comparators_.end();
    }

    virtual bool hermes_comparable() const override {
        return LeftTypeIdx < Comparables;
    }

    virtual int32_t hermes_compare(void* left, hermes::DocView* left_doc, ViewPtrHolder* left_ptr,
                                   void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) const override
    {
        if (right)
        {
            auto tag = arena::read_type_tag(right);
            auto ii = comparators_.find(tag);
            if (ii != comparators_.end())
            {
                T left_data_object(left, left_doc, left_ptr);
                return ii->second(left_data_object, right, right_doc, right_ptr);
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

    virtual bool hermes_equals(void* left, hermes::DocView* left_doc, ViewPtrHolder* left_ptr,
                          void* right, hermes::DocView* right_doc, ViewPtrHolder* right_ptr) const override
    {
        if (right)
        {
            auto tag = arena::read_type_tag(right);
            auto ii = equality_comparators_.find(tag);
            if (ii != equality_comparators_.end())
            {
                T left_data_object(left, left_doc, left_ptr);
                return ii->second(left_data_object, right, right_doc, right_ptr);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Types {} and {} are not equality-comparable", str(), get_type_reflection(tag).str()).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Right argument is nullptr").do_throw();
        }
    }

    virtual int32_t datatype_view_compare(const void*, const void*) const override {
        MEMORIA_MAKE_GENERIC_ERROR("Datatypes of type {} are not view-comparable" ).do_throw();
    }
};

}
