
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

namespace memoria {


class LDIdentifierView {
    const LDDocumentView* doc_;

    ld_::LDPtr<U8LinkedString> string_;

    friend class LDDocumentView;

public:
    LDIdentifierView():doc_(), string_({}) {}

    LDIdentifierView(const LDDocumentView* doc, ld_::LDPtr<U8LinkedString> string):
        doc_(doc), string_(string)
    {}

    U8StringView view() const {
        return string_.get(&doc_->arena_)->view();
    }
};

static inline std::ostream& operator<<(std::ostream& out, const LDIdentifierView& value) {
    out << value.view();
    return out;
}


}
