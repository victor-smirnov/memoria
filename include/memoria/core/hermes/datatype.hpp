
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/arena/string.hpp>
#include <memoria/core/arena/vector.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/common.hpp>


namespace memoria {
namespace hermes {

class HermesDocView;
class HermesDoc;

namespace detail {

class DatatypeData {
    arena::RelativePtr<arena::ArenaString> name_;
    arena::RelativePtr<arena::GenericVector> parameters_;
    arena::RelativePtr<arena::GenericVector> constructor_;
public:
    DatatypeData(arena::ArenaString* name):
        name_(name)
    {}

    arena::ArenaString* name() const {
        return name_.get();
    }

    arena::GenericVector* parameters() const {
        return parameters_.get();
    }

    void set_parameters(arena::GenericVector* params) {
        parameters_ = params;
    }

    arena::GenericVector* constructor() const {
        return constructor_.get();
    }

    void set_constructor(arena::GenericVector* args) {
        constructor_ = args;
    }

    bool has_constructor() const {
        return constructor_.is_not_null();
    }

    bool is_parametric() const {
        return parameters_.is_not_null();
    }
};

}


class Datatype: public HoldingView {
    friend class HermesDoc;
    friend class HermesDocView;
    friend class Value;
    friend class DocumentBuilder;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

protected:
    detail::DatatypeData* datatype_;
    HermesDocView* doc_;
public:
    Datatype() {}

    Datatype(void* dt, HermesDocView* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder), datatype_(reinterpret_cast<detail::DatatypeData*>(dt)),
        doc_(doc)
    {}

    ValuePtr as_value() const {
        return ValuePtr(Value(datatype_, doc_, ptr_holder_));
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state);

    void stringify_cxx(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state);

    bool is_simple_layout();

    StringValuePtr type_name();
    GenericArrayPtr set_constructor();
    GenericArrayPtr constructor();

    GenericArrayPtr type_parameters();
    DatatypePtr append_type_parameter(U8StringView name);
    DatatypePtr append_type_parameter(StringValuePtr name);

    void clear_parameters()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_parameters(nullptr);
    }

    void clear_constructor()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_constructor(nullptr);
    }

    template <typename DT>
    DataObjectPtr<DT> append_integral_parameter(DTTViewType<DT> view);

    bool has_constructor() const {
        assert_not_null();
        return datatype_->has_constructor();
    }

    bool is_parametric() const {
        assert_not_null();
        return datatype_->is_parametric();
    }

    void remove_constructor()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_constructor(nullptr);
    }

    void remove_parameters()
    {
        assert_not_null();
        assert_mutable();

        datatype_->set_parameters(nullptr);
    }

protected:

    void append_type_parameter(ValuePtr value);
    void append_constructor_argument(ValuePtr value);

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(datatype_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Datatype is null");
        }
    }

    void assert_mutable();
};

namespace detail {

template <>
struct ValueCastHelper<Datatype> {
    static DatatypePtr cast_to(void* addr, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept {
        return DatatypePtr(Datatype(
            addr,
            doc,
            ref_holder
        ));
    }
};

}


}}
