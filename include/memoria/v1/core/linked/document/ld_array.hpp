
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

namespace memoria {
namespace v1 {

class LDDArray {
public:
    using Array = ld_::Array;

private:
    friend class LDTypeDeclaration;
    friend class LDDMap;
    friend class LDDocument;

    using PtrHolder = typename ld_::LDArenaView::PtrHolderT;
    const LDDocumentView* doc_;
    Array array_;

public:
    LDDArray(): doc_(), array_() {}

    LDDArray(const LDDocumentView* doc, Array array) noexcept:
        doc_(doc), array_(array)
    {}

    LDDArray(const LDDocumentView* doc, PtrHolder ptr) noexcept:
        doc_(doc), array_(Array::get(&doc_->arena_, ptr))
    {}

    LDDArray(const LDDocumentView* doc, typename Array::ArrayPtr ptr, LDDValueTag) noexcept:
        doc_(doc), array_(Array::get(&doc_->arena_, ptr.get()))
    {}

    operator LDDValue() const noexcept;

    bool operator==(const LDDArray& other) const noexcept {
        return doc_->equals(other.doc_) && array_.ptr() == other.array_.ptr();
    }

    LDDValue get(size_t idx) const;

    void set_string(size_t idx, U8StringView value)
    {
        LDString str = doc_->make_mutable()->new_string(value);
        array_.access_checked(idx) = str.string_.get();
    }

    void set_integer(size_t idx, int64_t value)
    {
        LDDValue vv = doc_->make_mutable()->new_integer(value);
        array_.access_checked(idx) = vv.value_ptr_;
    }

    void set_double(size_t idx, double value)
    {
        LDDValue vv = doc_->make_mutable()->new_double(value);
        array_.access_checked(idx) = vv.value_ptr_;
    }

    void set_boolean(size_t idx, bool value)
    {
        LDDValue vv = doc_->make_mutable()->new_boolean(value);
        array_.access_checked(idx) = vv.value_ptr_;
    }

    LDDMap set_map(size_t idx);


    LDDArray set_array(size_t idx)
    {
        LDDArray vv = doc_->make_mutable()->new_array();
        array_.access_checked(idx) = vv.array_.ptr();
        return vv;
    }

    void set_null(size_t idx, bool value)
    {
        array_.access_checked(idx) = 0;
    }

    LDDValue set_sdn(size_t idx, U8StringView sdn)
    {
        LDDValue value = doc_->make_mutable()->parse_raw_value(sdn.begin(), sdn.end());
        array_.access_checked(idx) = value.value_ptr_;
        return value;
    }

    void add_string(U8StringView value)
    {
        LDString str = doc_->make_mutable()->new_string(value);
        array_.push_back(str.string_.get());
    }

    void add_integer(int64_t value)
    {
        LDDValue vv = doc_->make_mutable()->new_integer(value);
        array_.push_back(vv.value_ptr_);
    }

    void add_double(double value)
    {
        LDDValue vv = doc_->make_mutable()->new_double(value);
        array_.push_back(vv.value_ptr_);
    }

    void add_boolean(bool value)
    {
        LDDValue vv = doc_->make_mutable()->new_boolean(value);
        array_.push_back(vv.value_ptr_);
    }

    void add_null()
    {
        array_.push_back(0);
    }

    LDDValue add_document(const LDDocument& source)
    {
        ld_::assert_different_docs(doc_, &source);

        LDDocumentView* dst_doc = doc_->make_mutable();

        ld_::LDArenaAddressMapping mapping(source, *dst_doc);
        ld_::LDDPtrHolder ptr = source.value().deep_copy_to(dst_doc, mapping);

        array_.push_back(ptr);

        return LDDValue{doc_, ptr};
    }

    LDDValue set_document(size_t idx, const LDDocument& source)
    {
        ld_::assert_different_docs(doc_, &source);

        LDDocumentView* dst_doc = doc_->make_mutable();

        ld_::LDArenaAddressMapping mapping(source, *dst_doc);
        ld_::LDDPtrHolder ptr = source.value().deep_copy_to(dst_doc, mapping);
        array_.access_checked(idx) = ptr;

        return LDDValue{doc_, ptr};
    }

    LDDMap add_map();

    LDDArray add_array()
    {
        LDDArray vv = doc_->make_mutable()->new_array();
        array_.push_back(vv.array_.ptr());
        return vv;
    }

    LDDValue add_sdn(U8StringView sdn)
    {
        LDDValue value = doc_->make_mutable()->parse_raw_value(sdn.begin(), sdn.end());
        array_.push_back(value.value_ptr_);
        return value;
    }

    void remove(size_t idx){
        array_.remove(idx, 1);
    }

    void for_each(std::function<void(LDDValue)> fn) const;


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

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_dump(out, state, dump_state);
        }
        else {
            LDDumpFormatState simple_state = state.simple();
            do_dump(out, simple_state, dump_state);
        }

        return out;
    }

    size_t size() const noexcept {
        return array_.size();
    }

    bool is_simple_layout() const noexcept;


    ld_::LDPtr<Array::State> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    LDDocument clone(bool compactify = true) const {
        LDDValue vv = *this;
        return vv.clone(compactify);
    }

private:
    void do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;
};


static inline std::ostream& operator<<(std::ostream& out, const LDDArray& array)
{
    LDDumpFormatState format = LDDumpFormatState().simple();
    array.dump(out, format);
    return out;
}


template <>
struct DataTypeTraits<LDDArray> {
    static constexpr bool isDataType = true;
    using LDStorageType = NullType;
    using LDViewType = LDDArray;

    static void create_signature(SBuf& buf) {
        buf << "LDDArray";
    }
};

template <typename Arena>
auto ld_allocate_and_construct(const LDDArray*, Arena* arena)
{
    return LDDArray::Array::create_tagged_ptr(ld_tag_size<LDDArray>(), arena, 4);
}

template <typename Arena>
auto ld_allocate_and_construct(const LDDArray*, Arena* arena, size_t capacity)
{
    return LDDArray::Array::create_tagged_ptr(ld_tag_size<LDDArray>(), arena, capacity);
}

}}
