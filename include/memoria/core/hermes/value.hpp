
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
    HermesDocView* doc_;
    void* addr_;
public:
    Value() noexcept:
        doc_(), addr_()
    {}

    Value(void* addr, HermesDocView* doc_, ViewPtrHolder* ref_holder) noexcept :
        HoldingView(ref_holder),
        addr_(addr)
    {}

    virtual ~Value() noexcept = default;

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

    template <typename T>
    bool is_a(TypeTag<T>) const noexcept
    {
        assert_not_null();
        auto tag = TypeHashV<T>;
        auto value_tag = arena::read_type_tag(addr_);
        return tag == value_tag;
    }

    template <typename T>
    auto cast_to() {
        return cast_to(TypeTag<T>{});
    }

    template <typename T>
    auto cast_to(TypeTag<T>)
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
            get_type_reflection(value_tag).hermes_is_simple_layout(
                addr_, doc_, ptr_holder_
            );
        }

        return true;
    }


    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state)
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

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(addr_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Value is null");
        }
    }
};

}

template <typename T>
auto cast_to(ViewPtr<hermes::Value> val) {
    return val->cast_to(TypeTag<T>{});
}

}
