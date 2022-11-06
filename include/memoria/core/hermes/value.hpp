
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

#include <memoria/core/memory/object_pool.hpp>
#include <memoria/core/datatypes/datatype_ptrs.hpp>

#include <memoria/core/arena/arena.hpp>

#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/reflection/reflection.hpp>

namespace memoria {

template <>
class HoldingView<hermes::Value>: public hermes::TaggedHoldingView {

public:
    HoldingView(){}

    HoldingView(ViewPtrHolder* holder) noexcept:
        hermes::TaggedHoldingView(holder)
    {}
};


namespace hermes {

namespace detail {

template <typename T>
struct ValueCastHelper;

}



class Value: public HoldingView<Value> {
    using Base = HoldingView<Value>;
protected:
    static constexpr size_t TAG_ADDRESS     = 0;
    static constexpr size_t TAG_SMALL_VLAUE = 1;
    static constexpr size_t TAG_VIEW_PTR    = 2;
    // ... more tags

    mutable HermesCtr* doc_;

    using SmallValueHolder = TaggedValue;

    mutable ValueStorage value_storage_;

    using Base::get_tag;
    using Base::set_tag;

    friend class HermesCtr;

    template<typename, typename>
    friend class Map;

    template<typename>
    friend class Array;

public:
    Value() noexcept:
        doc_()
    {
        value_storage_.addr = nullptr;
        set_tag(0);
    }

    Value(void* addr, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept :
        Base(ref_holder),
        doc_(doc)
    {
        value_storage_.addr = addr;
        set_tag(0);
    }

    Value(const Value& other) noexcept :
        Base(other),
        doc_(other.doc_),
        value_storage_(other.value_storage_)
    {
        if (get_tag() == TAG_VIEW_PTR) {
            value_storage_.view_ptr->ref_copy();
        }
    }

    Value(Value&& other) noexcept :
        Base(std::move(other)),
        doc_(other.doc_),
        value_storage_(std::move(other.value_storage_))
    {
        if (get_tag() == TAG_VIEW_PTR) {
            other.value_storage_.view_ptr = nullptr;
        }
    }

    virtual ~Value() noexcept
    {
        if (get_tag() == TAG_VIEW_PTR)
        {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }
    }

    Value& operator=(const Value& other)
    {
        if (get_tag() == TAG_SMALL_VLAUE) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }

        Base::operator=(other);
        value_storage_ = other.value_storage_;

        if (get_tag() == TAG_SMALL_VLAUE) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->ref_copy();
            }
        }

        return *this;
    }

    Value& operator=(Value&& other)
    {
        if (get_tag() == TAG_SMALL_VLAUE) {
            if (value_storage_.view_ptr) {
                value_storage_.view_ptr->unref();
            }
        }

        Base::operator=(std::move(other));
        value_storage_ = std::move(other.value_storage_);

        if (get_tag() == TAG_SMALL_VLAUE) {
            other.value_storage_.view_ptr = nullptr;
        }

        return *this;
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, get_ptr_holder()->owner(), pool::DoRef{});
    }

    ValuePtr search(U8StringView query) const;
    ValuePtr search(U8StringView query, const IParameterResolver& params) const;

    bool is_convertible_to_plain_string() const
    {
        if (!is_null()) {
            auto tag = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag).is_convertible_to_plain_string();
        }
        return false;
    }

    U8String to_plain_string() const
    {
        assert_not_null();
        auto tag = arena::read_type_tag(value_storage_.addr);
        return get_type_reflection(tag).convert_to_plain_string(value_storage_.addr, doc_, get_ptr_holder());
    }

    template <typename DT>
    bool is_convertible_to() const {
        if (!is_null()) {
            auto src_tag = arena::read_type_tag(value_storage_.addr);
            auto to_tag = TypeHashV<DT>;
            return get_type_reflection(src_tag).is_convertible_to(to_tag);
        }
        return false;
    }

    template <typename DT>
    ValuePtr convert_to() const;

    ValuePtr as_value() const {
        return ValuePtr(*this);
    }

    bool is_null() const noexcept {
        return !value_storage_.addr;
    }

    bool is_not_null() const noexcept {
        return value_storage_.addr;
    }

    bool is_varchar() const noexcept {
        return is_a(TypeTag<Varchar>{});
    }

    bool is_generic_array() const noexcept {
        return is_a(TypeTag<Array<Value>>{});
    }

    bool is_generic_map() const noexcept {
        return is_a(TypeTag<Map<Varchar, Value>>{});
    }

    bool is_real() const noexcept {
        return is_a(TypeTag<Real>{});
    }

    bool is_double() const noexcept {
        return is_a(TypeTag<Double>{});
    }

    bool is_bigint() const noexcept {
        return is_a(TypeTag<BigInt>{});
    }

    bool is_boolean() const noexcept {
        return is_a(TypeTag<Boolean>{});
    }

    bool is_typed_value() const noexcept {
        return is_a(TypeTag<TypedValue>{});
    }

    bool is_datatype() const noexcept {
        return is_a(TypeTag<Datatype>{});
    }

    bool is_numeric() const noexcept {
        if (is_not_null()) {
            auto code = arena::read_type_tag(value_storage_.addr);
            return IsCodeInTheList<AllNumericDatatypes>::is_in(code);
        }
        return false;
    }

    bool is_numeric_real() const noexcept
    {
        if (is_not_null()) {
            auto code = arena::read_type_tag(value_storage_.addr);
            return IsCodeInTheList<RealNumericDatatypes>::is_in(code);
        }
        return false;
    }

    bool is_numeric_integer() const noexcept
    {
        if (is_not_null()) {
            auto code = arena::read_type_tag(value_storage_.addr);
            return IsCodeInTheList<IntegerNumericDatatypes>::is_in(code);
        }
        return false;
    }

    uint64_t type_tag() const noexcept {
        if (is_not_null()) {
            return arena::read_type_tag(value_storage_.addr);
        }
        return 0;
    }


    bool is_compareble_with(const ValuePtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            auto tag2 = arena::read_type_tag(other->value_storage_.addr);
            return get_type_reflection(tag1).hermes_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    int32_t compare(const ValuePtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag1).hermes_compare(
                    value_storage_.addr, doc_, get_ptr_holder(), other->value_storage_.addr, other->doc_, other->get_ptr_holder()
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    bool is_equals_compareble_with(const ValuePtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            auto tag2 = arena::read_type_tag(other->value_storage_.addr);
            return get_type_reflection(tag1).hermes_equals_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    bool equals(const ValuePtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(tag1).hermes_equals(
                    value_storage_.addr, doc_, get_ptr_holder(), other->value_storage_.addr, other->doc_, other->get_ptr_holder()
            );
        }
        else {
            return false;
        }
    }

    GenericArrayPtr as_generic_array() const;
    GenericMapPtr as_generic_map() const;
    DatatypePtr as_datatype() const;

    DataObjectPtr<Varchar> as_varchar() const;
    DataObjectPtr<Double> as_double() const;
    DataObjectPtr<BigInt> as_bigint() const;
    DataObjectPtr<Boolean> as_boolean() const;
    DataObjectPtr<Real> as_real() const;

    U8String type_str() const;

    bool operator<(const ValuePtr& other) const {
        return false;
    }

//    template <typename DT>
//    DTTViewType<DT> as_data_object() const {
//        return cast_to<DT>()->view();
//    }

    template <typename DT>
    DataObjectPtr<DT> as_data_object() const {
        return cast_to<DT>();
    }


    template <typename T>
    bool is_a(TypeTag<T>) const noexcept
    {
        if (MMA_LIKELY((bool)value_storage_.addr)) {
            auto tag = TypeHashV<T>;
            auto value_tag = arena::read_type_tag(value_storage_.addr);
            return tag == value_tag;
        }
        else {
            return false;
        }
    }

    template <typename T>
    auto cast_to() const {
        return cast_to(TypeTag<T>{});
    }

    template <typename T>
    auto cast_to(TypeTag<T>) const
    {
        assert_not_null();
        auto tag = TypeHashV<T>;
        auto value_tag = arena::read_type_tag(value_storage_.addr);

        if (value_tag == tag) {
            return detail::ValueCastHelper<T>::cast_to(value_storage_.addr, doc_, get_ptr_holder());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid value type tag: expected {}, actual {}", tag, value_tag).do_throw();
        }
    }

    bool is_simple_layout() const noexcept
    {
        if (!is_null()) {
            auto value_tag = arena::read_type_tag(value_storage_.addr);
            return get_type_reflection(value_tag).hermes_is_simple_layout(
                value_storage_.addr, doc_, get_ptr_holder()
            );
        }

        return true;
    }

    U8String to_string(const StringifyCfg& cfg = StringifyCfg{}) const
    {
        std::stringstream ss;
        stringify(ss, cfg);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        StringifyCfg cfg;
        cfg.set_spec(StringifySpec::simple());
        return to_string(cfg);
    }

    void stringify(std::ostream& out, const StringifyCfg& cfg = StringifyCfg{}) const
    {
        DumpFormatState state(cfg);
        stringify(out, state);
    }




    void stringify(std::ostream& out, DumpFormatState& state) const
    {
        if (MMA_UNLIKELY(is_null())) {
            out << "null";
        }
        else {
            auto value_tag = arena::read_type_tag(value_storage_.addr);
            get_type_reflection(value_tag).hermes_stringify_value(
                value_storage_.addr, doc_, get_ptr_holder(),
                out, state
            );
        }
    }

    PoolSharedPtr<HermesCtr> clone() const;

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(value_storage_.addr == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Value is null").do_throw();
        }
    }
};


using ValuePtr = ViewPtr<Value>;


std::ostream& operator<<(std::ostream& out, ValuePtr ptr);

struct Less {
    bool operator()(const ValuePtr& left, const ValuePtr& right) {
        return left->compare(right) < 0;
    }
};

struct Greater {
    bool operator()(const ValuePtr& left, const ValuePtr& right) {
        return left->compare(right) > 0;
    }
};

}

template <typename T>
auto cast_to(const hermes::ValuePtr& val) {
    return val->cast_to(TypeTag<T>{});
}
}


