
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/linked/document/ld_document.hpp>
#include <memoria/v1/core/linked/document/ld_string.hpp>
#include <memoria/v1/core/linked/document/ld_identifier.hpp>
#include <memoria/v1/core/linked/document/ld_map.hpp>
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_value.hpp>

namespace memoria {
namespace v1 {




class LDTypeDeclaration {
    using TypeDeclState = sdn2_::TypeDeclState;
    using TypeDeclPtr   = sdn2_::TypeDeclPtr;
    LDDocument* doc_;
    TypeDeclPtr state_;
public:
    LDTypeDeclaration(LDDocument* doc, TypeDeclPtr state):
        doc_(doc), state_(state)
    {}

    U8StringView name() const;

    size_t params() const;

    LDTypeDeclaration get_type_declration(size_t idx) const;
    LDTypeDeclaration add_type_declaration(U8StringView name);
    void remove_type_declaration(size_t idx);

    size_t constructor_args() const;

    LDDValue get_constructor_arg(size_t idx) const;
    LDString add_string_constructor_arg(U8StringView value);
    void add_double_constructor_arg(double value);
    void add_integer_constructor_arg(int64_t value);
    LDDArray add_array_constructor_arg();
    LDDMap add_map_constructor_arg();

    void remove_constructor_arg(size_t idx);

private:

    TypeDeclState* state() {
        return state_.get(&doc_->arena_);
    }

    const TypeDeclState* state() const {
        return state_.get(&doc_->arena_);
    }
};


}}
