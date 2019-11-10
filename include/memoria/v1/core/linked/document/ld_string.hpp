
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

namespace memoria {
namespace v1 {



class LDString {
    const LDDocumentView* doc_;
    ld_::LDPtr<U8LinkedString> string_;

    friend class LDTypeDeclaration;
    friend class LDDArray;
    friend class LDDMap;

public:
    LDString(): doc_(), string_({}) {}

    LDString(const LDDocumentView* doc, ld_::LDPtr<U8LinkedString> string):
        doc_(doc), string_(string)
    {}

    U8StringView view() const {
        return string_.get(&doc_->arena_)->view();
    }

    operator LDDValue() const;

    LDDocument clone(bool compactify = true) const;

    ld_::LDPtr<U8LinkedString> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;
};

}}
