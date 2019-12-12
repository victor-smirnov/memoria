
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



class LDDValueView {
    const LDDocumentView* doc_;
    ld_::LDDPtrHolder value_ptr_;
    LDDValueTag type_tag_;

    friend class LDDocument;
    friend class LDDocumentView;
    friend class LDDMapView;
    friend class LDDArrayView;
    friend class LDTypeDeclarationView;

    template <typename>
    friend struct DataTypeOperationsImpl;

public:
    LDDValueView() noexcept : doc_(), value_ptr_(), type_tag_() {}

    LDDValueView(const LDDocumentView* doc, ld_::LDDPtrHolder value_ptr) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(value_ptr ? get_tag(value_ptr) : 0)
    {}

    LDDValueView(const LDDocumentView* doc, ld_::LDDPtrHolder value_ptr, LDDValueTag tag) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(tag)
    {}

    bool operator==(const LDDValueView& other) const noexcept {
        return doc_->equals(other.doc_) && value_ptr_ == other.value_ptr_;
    }


    LDDMapView as_map() const;
    LDDArrayView as_array() const;
    LDTypeDeclarationView as_type_decl() const;
    LDDTypedValueView as_typed_value() const;

    template <typename T>
    DTTLDViewType<T> cast_as() const
    {
        ld_::ldd_assert_tag<T>(type_tag_);
        return MakeLDView<T>::process(doc_, value_ptr_, type_tag_);
    }

    template <typename T>
    DTTLDViewType<T> unchecked_cast_as() const
    {
        return MakeLDView<T>::process(doc_, value_ptr_, type_tag_);
    }

    LDStringView as_varchar() const
    {
        ld_::ldd_assert_tag<Varchar>(type_tag_);
        return LDStringView(doc_, value_ptr_);
    }

    DTTViewType<BigInt> as_bigint() const
    {
        ld_::ldd_assert_tag<BigInt>(type_tag_);
        return *ld_::LDPtr<DTTLDStorageType<BigInt>>(value_ptr_).get(&doc_->arena_);
    }

    DTTViewType<Double> as_double() const
    {
        ld_::ldd_assert_tag<Double>(type_tag_);
        return *ld_::LDPtr<DTTLDStorageType<Double>>(value_ptr_).get(&doc_->arena_);
    }

    DTTViewType<Real> as_real() const
    {
        ld_::ldd_assert_tag<Real>(type_tag_);
        return *ld_::LDPtr<DTTLDStorageType<Real>>(value_ptr_).get(&doc_->arena_);
    }

    DTTViewType<Boolean> as_boolean() const
    {
        ld_::ldd_assert_tag<Boolean>(type_tag_);
        return *ld_::LDPtr<DTTLDStorageType<Boolean>>(value_ptr_).get(&doc_->arena_);
    }

    bool is_null() const noexcept {
        return value_ptr_ == 0;
    }

    bool is_bigint() const noexcept {
        return type_tag_ == ld_tag_value<BigInt>();
    }

    bool is_real() const noexcept {
        return type_tag_ == ld_tag_value<Real>();
    }

    bool is_double() const noexcept {
        return type_tag_ == ld_tag_value<Double>();
    }

    bool is_boolean() const noexcept {
        return type_tag_ == ld_tag_value<Boolean>();
    }


    bool is_varchar() const noexcept {
        return type_tag_ == ld_tag_value<Varchar>();
    }

    bool is_array() const noexcept {
        return type_tag_ == ld_tag_value<LDArray>();
    }

    bool is_map() const noexcept {
        return type_tag_ == ld_tag_value<LDMap>();
    }

    bool is_type_decl() const noexcept {
        return type_tag_ == ld_tag_value<LDTypeDeclaration>();
    }

    bool is_typed_value() const noexcept {
        return type_tag_ == ld_tag_value<LDTypedValue>();
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

static inline std::ostream& operator<<(std::ostream& out, const LDDValueView& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    value.dump(out, format);
    return out;
}

template <typename T>
DTTLDViewType<T> cast_as(const LDDValueView& view) {
    return view.template cast_as<T>();
}

template <typename T>
DTTLDViewType<T> unchecked_cast_as(const LDDValueView& view) {
    return view.template unchecked_cast_as<T>();
}

}}
