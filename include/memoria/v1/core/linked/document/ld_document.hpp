
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

#include <memoria/v1/core/linked/document/ld_common.hpp>

namespace memoria {
namespace v1 {



class LDDocument {
    using ValueMap = sdn2_::ValueMap;
    using StringSet = sdn2_::StringSet;
    using Array = sdn2_::Array;

    SDN2Arena arena_;
    SDN2Ptr<sdn2_::DocumentState> doc_;

    friend class LDDocumentBuilder;
    friend class LDDMap;
    friend class LDDArray;
    friend class LDTypeName;
    friend class LDDataTypeParam;
    friend class LDDataTypeCtrArg;
    friend class LDTypeDeclaration;
    friend class LDDValue;
    friend class LDString;
    friend class LDIdentifier;

public:
    LDDocument() {
        doc_ = allocate<sdn2_::DocumentState>(&arena_, sdn2_::DocumentState{0, 0, 0});
    }

    LDDValue value() const;

    void set(U8StringView string);
    void set(int64_t value);
    void set(double value);

    LDDMap set_map();
    LDDArray set_array();

    static LDDocument parse(U8StringView view) {
        return parse(view.begin(), view.end());
    }

    static LDDocument parse(U8StringView::const_iterator start, U8StringView::const_iterator end);

    std::ostream& dump(std::ostream& out) const;

    LDString new_string(U8StringView view);
    LDIdentifier new_identifier(U8StringView view);

    LDDValue new_integer(int64_t value);
    LDDValue new_double(double value);
    LDDArray new_array(Span<LDDValue> span);
    LDDArray new_array();
    LDDMap new_map();

    void set_value(LDDValue value);

private:




    SDN2Ptr<U8LinkedString> intern(U8StringView view);

    sdn2_::DocumentState* state() {
        return doc_.get(&arena_);
    }

    const sdn2_::DocumentState* state() const {
        return doc_.get(&arena_);
    }

    void set_tag(SDN2PtrHolder ptr, LDDValueTag tag)
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        *tag_ptr.get(&arena_) = tag;
    }
};



}}
