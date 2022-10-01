
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>
#include <memoria/core/hermes/datatype.hpp>

namespace memoria {
namespace hermes {

namespace detail {

class TypedValueData {
    arena::RelativePtr<DatatypeData> datatype_;
    arena::RelativePtr<void> constructor_;
public:
    TypedValueData(
        DatatypeData* datatype,
        void* constructor
    ): datatype_(datatype), constructor_(constructor)
    {}

    DatatypeData* datatype() const {
        return datatype_.get();
    }

    void* constructor() const {
        return constructor_.get();
    }
};

}

class TypedValue: public HoldingView {
    detail::TypedValueData* tv_;
    HermesDocView* doc_;
public:
    TypedValue() noexcept:
        tv_(), doc_()
    {}

    TypedValue(void* tv, HermesDocView* doc, ViewPtrHolder* ptr_holder) noexcept:
        HoldingView(ptr_holder),
        tv_(reinterpret_cast<detail::TypedValueData*>(tv)), doc_(doc)
    {}

    ValuePtr as_value() {
        return ValuePtr(Value(tv_, doc_, ptr_holder_));
    }

    bool is_null() const {
        return tv_ == nullptr;
    }

    bool is_null_not() const {
        return tv_ != nullptr;
    }

    ValuePtr constructor()
    {
        assert_not_null();
        return ValuePtr(Value(tv_->constructor(), doc_, ptr_holder_));
    }

    DatatypePtr datatype()
    {
        assert_not_null();
        return DatatypePtr(Datatype(tv_->datatype(), doc_, ptr_holder_));
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state);

    bool is_simple_layout()
    {
        assert_not_null();
        return constructor()->is_simple_layout() && datatype()->is_simple_layout();
    }


private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(tv_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("TypedValue is null");
        }
    }
};

}}
