
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
#include <memoria/v1/core/linked/document/ld_value.hpp>
#include <memoria/v1/core/linked/document/ld_type_decl.hpp>

namespace memoria {
namespace v1 {

class LDDTypedValue {
    using State = sdn2_::TypedValueState;

    LDDocumentView* doc_;
    SDN2Ptr<State> state_;

    friend class LDTypeDeclaration;

public:
    LDDTypedValue(): doc_(), state_({}) {}

    LDDTypedValue(LDDocumentView* doc, SDN2Ptr<State> state):
        doc_(doc), state_(state)
    {}

    LDTypeDeclaration type() const
    {
        return LDTypeDeclaration{const_cast<LDDocumentView*>(doc_), state()->type_decl};
    }

    LDDValue constructor() const
    {
        return LDTypeDeclaration{const_cast<LDDocumentView*>(doc_), state()->value_ptr};
    }

    operator LDDValue() const {
        return LDDValue(doc_, state_.get());
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    bool is_simple_layout() const noexcept
    {
        return type().is_simple_layout() && constructor().is_simple_layout();
    }

private:
    const State* state() const {
        return state_.get(doc_->arena_);
    }
};

}}
