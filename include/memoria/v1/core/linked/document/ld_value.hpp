
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

namespace memoria {
namespace v1 {



class LDDValue {
    const LDDocumentView* doc_;
    ld_::LDDPtrHolder value_ptr_;
    LDDValueTag type_tag_;

    friend class LDDocument;
    friend class LDDocumentView;
    friend class LDDMap;
    friend class LDDArray;
    friend class LDTypeDeclaration;

    template <typename>
    friend struct DataTypeOperationsImpl;

public:
    LDDValue() noexcept : doc_(), value_ptr_(), type_tag_() {}

    LDDValue(const LDDocumentView* doc, ld_::LDDPtrHolder value_ptr) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(value_ptr ? get_tag(value_ptr) : 0)
    {}

    LDDValue(const LDDocumentView* doc, ld_::LDDPtrHolder value_ptr, LDDValueTag tag) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(tag)
    {}

    bool operator==(const LDDValue& other) const noexcept {
        return doc_->equals(other.doc_) && value_ptr_ == other.value_ptr_;
    }


    LDDMap as_map() const;
    LDDArray as_array() const;
    LDTypeDeclaration as_type_decl() const;
    LDDTypedValue as_typed_value() const;

    LDString as_string() const
    {
        ld_::ldd_assert_tag<LDString>(type_tag_);
        return LDString(doc_, value_ptr_);
    }

    LDInteger as_integer() const
    {
        ld_::ldd_assert_tag<LDInteger>(type_tag_);
        return *ld_::LDPtr<ld_::LDIntegerStorage>(value_ptr_).get(&doc_->arena_);
    }

    LDDouble as_double() const
    {
        ld_::ldd_assert_tag<LDDouble>(type_tag_);
        return *ld_::LDPtr<ld_::LDDoubleStorage>(value_ptr_).get(&doc_->arena_);
    }

    LDBoolean as_boolean() const
    {
        ld_::ldd_assert_tag<LDBoolean>(type_tag_);
        return *ld_::LDPtr<ld_::LDBooleanStorage>(value_ptr_).get(&doc_->arena_);
    }

    bool is_null() const noexcept {
        return value_ptr_ == 0;
    }

    bool is_integer() const noexcept {
        return type_tag_ == ld_tag_value<LDInteger>();
    }

    bool is_double() const noexcept {
        return type_tag_ == ld_tag_value<LDDouble>();
    }

    bool is_boolean() const noexcept {
        return type_tag_ == ld_tag_value<LDBoolean>();
    }

    bool is_string() const noexcept {
        return type_tag_ == ld_tag_value<LDString>();
    }

    bool is_array() const noexcept {
        return type_tag_ == ld_tag_value<LDDArray>();
    }

    bool is_map() const noexcept {
        return type_tag_ == ld_tag_value<LDDMap>();
    }

    bool is_type_decl() const noexcept {
        return type_tag_ == ld_tag_value<LDTypeDeclaration>();
    }

    bool is_typed_value() const noexcept {
        return type_tag_ == ld_tag_value<LDDTypedValue>();
    }

    bool is_simple_layout() const noexcept;


    LDDValueTag tag() const {
        return type_tag_;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*doc_);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out) const {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        return dump(out, state, dump_state);
    }

    U8String to_standard_string() const {
        LDDumpFormatState state = LDDumpFormatState().simple();
        std::stringstream ss;
        dump(ss, state);
        return ss.str();
    }

    ld_::LDDPtrHolder deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    LDDocument clone(bool compactify = true) const;

private:
    LDDValueTag get_tag(ld_::LDDPtrHolder ptr) const noexcept
    {
        return ld_::ldd_get_tag(&doc_->arena_, ptr);
    }
};

static inline std::ostream& operator<<(std::ostream& out, const LDDValue& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    value.dump(out, format);
    return out;
}


}}
