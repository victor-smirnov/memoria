
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

#include <memoria/core/linked/document/ld_document.hpp>
#include <memoria/core/linked/document/ld_value.hpp>
#include <memoria/core/linked/document/ld_type_decl.hpp>

namespace memoria {

class LDDTypedValueView {
    using State = ld_::TypedValueState;

    const LDDocumentView* doc_;
    ld_::LDPtr<State> state_;

    friend class LDTypeDeclarationView;

public:
    LDDTypedValueView(): doc_(), state_({}) {}

    LDDTypedValueView(const LDDocumentView* doc, ld_::LDPtr<State> state):
        doc_(doc), state_(state)
    {}

    LDTypeDeclarationView type() const
    {
        return LDTypeDeclarationView{doc_, state()->type_decl};
    }

    LDDValueView constructor() const
    {
        return LDTypeDeclarationView{doc_, state()->value_ptr};
    }

    operator LDDValueView() const {
        return LDDValueView(doc_, state_.get());
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*doc_);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    bool is_simple_layout() const noexcept
    {
        return type().is_simple_layout() && constructor().is_simple_layout();
    }

    ld_::LDPtr<State> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

private:
    const State* state() const {
        return state_.get(&doc_->arena_);
    }
};

static inline std::ostream& operator<<(std::ostream& out, const LDDTypedValueView& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    value.dump(out, format);
    return out;
}

template <>
struct DataTypeTraits<LDTypedValue> {
    static constexpr bool isDataType = true;
    using LDStorageType = NullType;
    using LDViewType = LDDTypedValueView;

    static void create_signature(SBuf& buf) {
        buf << "LDTypedValue";
    }
};

}
