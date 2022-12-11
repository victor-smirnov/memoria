
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
#include <memoria/core/datatypes/varchars/varchars.hpp>

#include <memoria/core/arena/arena.hpp>

#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/reflection/reflection.hpp>

namespace memoria {

template <>
class HoldingView<hermes::ObjectView>: public hermes::TaggedHoldingView {

public:
    HoldingView(){}

    HoldingView(LWMemHolder* holder) noexcept:
        hermes::TaggedHoldingView(holder)
    {}
};


namespace hermes {

class GenericDataObjectImpl;

class ObjectView: public HoldingView<ObjectView> {
    using Base = HoldingView<ObjectView>;
protected:
    using SmallValueHolder = TaggedValue;

    mutable ValueStorage storage_;

    friend class HermesCtr;

    template<typename, typename>
    friend class MapView;

    template<typename>
    friend class ArrayView;

    friend class GenericDataObjectImpl;

    template <typename, ObjectCasters>
    friend struct ObjectCaster;

public:
    ObjectView() noexcept
    {
        storage_.addr = nullptr;
        set_tag(ValueStorageTag::VS_TAG_ADDRESS);
    }

    ObjectView(LWMemHolder* ref_holder, void* addr) noexcept :
        Base(ref_holder)
    {
        storage_.addr = addr;
        set_vs_tag(ValueStorageTag::VS_TAG_ADDRESS);
    }

    ObjectView(LWMemHolder* ref_holder, void* addr, MoveOwnershipTag) noexcept :
        Base(ref_holder)
    {
        storage_.addr = addr;
        set_vs_tag(ValueStorageTag::VS_TAG_ADDRESS);
    }


    ObjectView(LWMemHolder* ref_holder, const TaggedValue& tagged_value) noexcept :
        Base(ref_holder)
    {
        storage_.small_value = tagged_value;
        set_vs_tag(ValueStorageTag::VS_TAG_SMALL_VALUE);
    }

    ObjectView(LWMemHolder* ref_holder, ValueStorageTag vs_tag, ValueStorage storage) noexcept :
        Base(ref_holder)
    {
        storage_ = storage;
        set_vs_tag(vs_tag);

        if (vs_tag == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            storage_.view_ptr->ref_copy();
        }
    }

    ObjectView(const ObjectView& other) noexcept :
        Base(other),
        storage_(other.storage_)
    {
        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            storage_.view_ptr->ref_copy();
        }
    }

    ObjectView(ObjectView&& other) noexcept :
        Base(std::move(other)),
        storage_(std::move(other.storage_))
    {
        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            other.storage_.view_ptr = nullptr;
        }
    }

    virtual ~ObjectView() noexcept
    {
        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (storage_.view_ptr) {
                storage_.view_ptr->unref();
            }
        }
    }

    ObjectView& operator=(const ObjectView& other)
    {
        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (storage_.view_ptr) {
                storage_.view_ptr->unref();
            }
        }

        Base::operator=(other);
        storage_ = other.storage_;

        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (storage_.view_ptr) {
                storage_.view_ptr->ref_copy();
            }
        }

        return *this;
    }

    ObjectView& operator=(ObjectView&& other)
    {
        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (storage_.view_ptr) {
                storage_.view_ptr->unref();
            }
        }

        Base::operator=(std::move(other));
        storage_ = std::move(other.storage_);

        if (get_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            other.storage_.view_ptr = nullptr;
        }

        return *this;
    }

    bool is_detached() const {
        return get_vs_tag() != ValueStorageTag::VS_TAG_SMALL_VALUE;
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();        
        return PoolSharedPtr<HermesCtr>(
                    get_mem_holder()->ctr(),
                    get_mem_holder()->owner(),
                    pool::DoRef{}
        );
    }

    Object search(U8StringView query) const;
    Object search(U8StringView query, const IParameterResolver& params) const;

    Object search2(U8StringView query) const;
    Object search2(U8StringView query, const IParameterResolver& params) const;

    bool is_convertible_to_plain_string() const
    {
        if (!is_null()) {
            auto tag = get_type_tag();
            return get_type_reflection(tag).is_convertible_to_plain_string();
        }
        return false;
    }

    U8String to_plain_string() const
    {
        assert_not_null();
        auto tag = get_type_tag();
        return get_type_reflection(tag).convert_to_plain_string(
            get_mem_holder(), get_vs_tag(), storage_
        );
    }

    template <typename DT>
    bool is_convertible_to() const
    {
        if (!is_null()) {
            auto src_tag = get_type_tag();
            auto to_tag = ShortTypeCode::of<DT>();
            return src_tag == to_tag || get_type_reflection(src_tag).is_convertible_to(to_tag);
        }
        return false;
    }

    template <typename DT>
    Object convert_to() const;

    Object as_object() const {
        return Object(*this);
    }

    bool is_null() const noexcept {
        return get_mem_holder() == nullptr ||
                (get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS
                 && storage_.addr == nullptr);
    }

    bool is_not_null() const noexcept {
        return !is_null();
    }

    bool is_varchar() const noexcept {
        return is_a(TypeTag<Varchar>{});
    }

    bool is_array() const noexcept {
        assert_not_null();
        return get_type_tag().descriptor() == HERMES_OBJECT_ARRAY;
    }

    bool is_map() const noexcept {
        assert_not_null();
        return get_type_tag().descriptor() == HERMES_OBJECT_MAP;
    }

    bool is_data() const noexcept {
        assert_not_null();
        return get_type_tag().descriptor() == 0;
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
            auto code = get_type_tag();
            return IsCodeInTheList<AllNumericDatatypes>::is_in(code);
        }
        return false;
    }

    bool is_numeric_real() const noexcept
    {
        if (is_not_null()) {
            auto code = get_type_tag();
            return IsCodeInTheList<RealNumericDatatypes>::is_in(code);
        }
        return false;
    }

    bool is_numeric_integer() const noexcept
    {
        if (is_not_null()) {
            auto code = get_type_tag();
            return IsCodeInTheList<IntegerNumericDatatypes>::is_in(code);
        }
        return false;
    }

    ShortTypeCode type_tag() const noexcept {
        if (is_not_null()) {
            return get_type_tag();
        }
        return ShortTypeCode::nullv();
    }


    bool is_compareble_with(const Object& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = get_type_tag();
            auto tag2 = other->get_type_tag();
            return get_type_reflection(tag1).hermes_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    int32_t compare(const Object& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = get_type_tag();
            return get_type_reflection(tag1).hermes_compare(
                    get_mem_holder(),
                    get_vs_tag(),
                    storage_,
                    other->get_mem_holder(),
                    other->get_vs_tag(),
                    other->storage_
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    bool is_equals_compareble_with(const Object& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = get_type_tag();
            auto tag2 = other->get_type_tag();
            return get_type_reflection(tag1).hermes_equals_comparable_with(tag2);
        }
        else {
            return false;
        }
    }

    bool equals(const Object& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            auto tag1 = get_type_tag();
            return get_type_reflection(tag1).hermes_equals(
                    get_mem_holder(),
                    get_vs_tag(),
                    storage_,
                    other->get_mem_holder(),
                    other->get_vs_tag(),
                    other->storage_
            );
        }
        else {
            return false;
        }
    }


    GenericArrayPtr as_generic_array() const;
    GenericMapPtr as_generic_map() const;
    Datatype as_datatype() const;

    DTView<Varchar> as_varchar() const;
    DTView<Double>  as_double() const;
    DTView<BigInt>  as_bigint() const;
    DTView<Boolean> as_boolean() const;
    DTView<Real>    as_real() const;

    int64_t to_i64() const;
    int64_t to_i32() const;
    U8String to_str() const;
    bool to_bool() const;
    double to_d64() const;
    float to_f32() const;

    bool is_object_map() const {
        return is_a<ObjectMap>();
    }

    bool is_tiny_object_map() const {
        return is_a<TinyObjectMap>();
    }

    bool is_object_array() const {
        return is_a<ObjectArray>();
    }

    ObjectMap as_object_map() const;
    TinyObjectMap as_tiny_object_map() const;
    ObjectArray as_object_array() const;


    U8String type_str() const;

    bool operator<(const Object& other) const {
        return false;
    }

    template <typename DT>
    DTView<DT> as_data_object() const {
        return cast_to<DT>();
    }

    template <typename DT, typename Fn>
    decltype(auto) with_data_object(Fn&& fn) const {
        return fn(as_data_object<DT>()->view());
    }

    template <typename T>
    bool is_a(TypeTag<T>) const noexcept
    {
        if (is_not_null()) {
            auto tag = ShortTypeCode::of<T>();
            auto value_tag = get_type_tag();
            return tag == value_tag;
        }
        else {
            return false;
        }
    }

    template <typename T>
    bool is_a() const noexcept
    {
        return is_a<T>(TypeTag<T>{});
    }

    template <typename T>
    auto cast_to() const {
        return cast_to(TypeTag<T>{});
    }

    template <typename T>
    auto cast_to(TypeTag<T>) const;

    bool is_simple_layout() const noexcept
    {
        if (!is_null())
        {
            if (get_tag() == VS_TAG_ADDRESS) {
                auto value_tag = get_type_tag();
                return get_type_reflection(value_tag).hermes_is_simple_layout(
                            get_mem_holder(), storage_.addr
                );
            }
            else {
                return true;
            }
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
        cfg.set_spec(StringifySpec());
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
            auto value_tag = get_type_tag();
            get_type_reflection(value_tag).hermes_stringify_value(
                get_mem_holder(), get_vs_tag(), storage_,
                out, state
            );
        }
    }

    PoolSharedPtr<HermesCtr> clone() const;

private:
    ShortTypeCode get_type_tag() const noexcept
    {
        if (get_vs_tag() == ValueStorageTag::VS_TAG_ADDRESS) {
            return arena::read_type_tag(storage_.addr);
        }
        else if (get_vs_tag() == ValueStorageTag::VS_TAG_SMALL_VALUE) {
            return storage_.small_value.tag();
        }
        else if (get_vs_tag() == ValueStorageTag::VS_TAG_GENERIC_VIEW) {
            if (storage_.view_ptr) {
                return storage_.view_ptr->tag();
            }
        }

        return ShortTypeCode::nullv();
    }

    ValueStorageTag get_vs_tag() const noexcept {
        return static_cast<ValueStorageTag>(this->get_tag());
    }


    void set_vs_tag(ValueStorageTag tag) noexcept {
        this->set_tag(static_cast<size_t>(tag));
    }

    void assert_not_null() const
    {
        if (is_null()) {
            MEMORIA_MAKE_GENERIC_ERROR("Object is null").do_throw();
        }
    }

    void* addr() const {
        return storage_.addr;
    }

    void ref() {
        get_mem_holder()->ref_copy();
    }

    void unref() {
        get_mem_holder()->unref();
    }
};


std::ostream& operator<<(std::ostream& out, Object ptr);

struct Less {
    bool operator()(const Object& left, const Object& right) {
        return left->compare(right) < 0;
    }
};

struct Greater {
    bool operator()(const Object& left, const Object& right) {
        return left->compare(right) > 0;
    }
};


}

template <typename T>
auto cast_to(const hermes::Object& val) {
    return val->cast_to(TypeTag<T>{});
}
}


