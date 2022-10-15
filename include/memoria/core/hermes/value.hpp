
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
namespace hermes {

namespace detail {

template <typename T>
struct ValueCastHelper;

}

class Value: public HoldingView {
protected:
    mutable HermesDocView* doc_;
    mutable void* addr_;

    friend class HermesDocView;

    template<typename, typename>
    friend class Map;

    template<typename>
    friend class Array;

public:
    Value() noexcept:
        doc_(), addr_()
    {}

    Value(void* addr, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept :
        HoldingView(ref_holder),
        doc_(doc),
        addr_(addr)
    {}

    virtual ~Value() noexcept = default;

    PoolSharedPtr<HermesDocView> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesDocView>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ValuePtr as_value() const {
        return ValuePtr(*this);
    }

    bool is_null() const noexcept {
        return !addr_;
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

    GenericArrayPtr as_generic_array() const;
    GenericMapPtr as_generic_map() const;
    DataObjectPtr<Varchar> as_varchar() const;
    DataObjectPtr<Double> as_double() const;
    DataObjectPtr<BigInt> as_bigint() const;
    DataObjectPtr<Boolean> as_boolean() const;

    template <typename DT>
    DTTViewType<DT> as_data_object() const {
        return cast_to<DT>()->view();
    }

    template <typename T>
    bool is_a(TypeTag<T>) const noexcept
    {
        if (MMA_LIKELY((bool)addr_)) {
            auto tag = TypeHashV<T>;
            auto value_tag = arena::read_type_tag(addr_);
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
        auto value_tag = arena::read_type_tag(addr_);

        if (value_tag == tag) {
            return detail::ValueCastHelper<T>::cast_to(addr_, doc_, ptr_holder_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid value type tag: expected {}, actual {}", tag, value_tag).do_throw();
        }
    }

    bool is_simple_layout() const noexcept
    {
        if (!is_null()) {
            auto value_tag = arena::read_type_tag(addr_);
            return get_type_reflection(value_tag).hermes_is_simple_layout(
                addr_, doc_, ptr_holder_
            );
        }

        return true;
    }

    U8String to_string() const
    {
        DumpFormatState fmt = DumpFormatState().simple();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        DumpFormatState fmt = DumpFormatState();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out) const
    {
        DumpFormatState state;
        DumpState dump_state(*doc_);
        stringify(out, state, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& format) const
    {
        DumpState dump_state(*doc_);
        stringify(out, format, dump_state);
    }



    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const
    {
        if (is_null()) {
            out << "null";
        }
        else {
            auto value_tag = arena::read_type_tag(addr_);
            get_type_reflection(value_tag).hermes_stringify_value(
                addr_, doc_, ptr_holder_,
                out, state, dump_state
            );
        }
    }

    PoolSharedPtr<HermesDocView> clone() const;

    bool equals(const ValuePtr& ptr) const {
        return addr_ == ptr->addr_;
    }

    bool equals(const Value& vv) const {
        return addr_ == vv.addr_;
    }

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(addr_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Value is null").do_throw();
        }
    }
};


using ValuePtr = ViewPtr<Value>;


std::ostream& operator<<(std::ostream& out, ValuePtr ptr);

}

template <typename T>
auto cast_to(const hermes::ValuePtr& val) {
    return val->cast_to(TypeTag<T>{});
}

}
