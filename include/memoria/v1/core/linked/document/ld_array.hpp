
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

class LDDArrayView {
public:
    using Array = ld_::Array;

private:
    friend class LDTypeDeclarationView;
    friend class LDDMapView;
    friend class LDDocument;

    using PtrHolder = typename ld_::LDArenaView::PtrHolderT;
    const LDDocumentView* doc_;
    Array array_;

public:
    LDDArrayView(): doc_(), array_() {}

    LDDArrayView(const LDDocumentView* doc, Array array) noexcept:
        doc_(doc), array_(array)
    {}

    LDDArrayView(const LDDocumentView* doc, PtrHolder ptr) noexcept:
        doc_(doc), array_(Array::get(&doc_->arena_, ptr))
    {}

    LDDArrayView(const LDDocumentView* doc, typename Array::ArrayPtr ptr, LDDValueTag) noexcept:
        doc_(doc), array_(Array::get(&doc_->arena_, ptr.get()))
    {}

    operator LDDValueView() const noexcept;

    bool operator==(const LDDArrayView& other) const noexcept {
        return doc_->equals(other.doc_) && array_.ptr() == other.array_.ptr();
    }

    LDDValueView get(size_t idx) const;


    template <typename T, typename... Args>
    LDDValueView set_value(size_t idx, Args&&... args)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();
        auto vv = mutable_doc->template new_value<T>(std::forward<Args>(args)...);
        array_.access_checked(idx) = vv;
        return LDDValueView {doc_, vv, ld_tag_value<T>()};
    }

    void set_varchar(size_t idx, DTTViewType<Varchar> value)
    {
        set_value<Varchar>(idx, value);
    }

    void set_bigint(size_t idx, int64_t value)
    {
        set_value<BigInt>(idx, value);
    }

    void set_double(size_t idx, double value)
    {
        set_value<Double>(idx, value);
    }

    void set_boolean(size_t idx, bool value)
    {
        set_value<Boolean>(idx, value);
    }

    LDDMapView set_map(size_t idx);


    LDDArrayView set_array(size_t idx)
    {
        return set_value<LDArray>(idx).as_array();
    }

    void set_null(size_t idx, bool value)
    {
        array_.access_checked(idx) = 0;
    }

    LDDValueView set_sdn(size_t idx, U8StringView sdn)
    {
        LDDValueView value = doc_->make_mutable()->parse_raw_value(sdn.begin(), sdn.end());
        array_.access_checked(idx) = value.value_ptr_;
        return value;
    }


    template <typename T, typename... Args>
    LDDValueView add_value(Args&&... args)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();
        auto vv = mutable_doc->template new_value<T>(std::forward<Args>(args)...);
        array_.push_back(vv);
        return LDDValueView{doc_, vv, ld_tag_value<T>()};
    }

    void add_varchar(DTTViewType<Varchar> value)
    {
        add_value<Varchar>(value);
    }

    void add_bigint(int64_t value)
    {
        add_value<BigInt>(value);
    }

    void add_double(double value)
    {
        add_value<Double>(value);
    }

    void add_boolean(bool value)
    {
        add_value<Boolean>(value);
    }

    void add_null()
    {
        array_.push_back(0);
    }

    LDDValueView add_document(const LDDocument& source)
    {
        ld_::assert_different_docs(doc_, &source);

        LDDocumentView* dst_doc = doc_->make_mutable();

        ld_::LDArenaAddressMapping mapping(source, *dst_doc);
        ld_::LDDPtrHolder ptr = source.value().deep_copy_to(dst_doc, mapping);

        array_.push_back(ptr);

        return LDDValueView{doc_, ptr};
    }

    LDDValueView set_document(size_t idx, const LDDocument& source)
    {
        ld_::assert_different_docs(doc_, &source);

        LDDocumentView* dst_doc = doc_->make_mutable();

        ld_::LDArenaAddressMapping mapping(source, *dst_doc);
        ld_::LDDPtrHolder ptr = source.value().deep_copy_to(dst_doc, mapping);
        array_.access_checked(idx) = ptr;

        return LDDValueView{doc_, ptr};
    }

    LDDMapView add_map();

    LDDArrayView add_array()
    {
        return add_value<LDArray>().as_array();
    }

    LDDValueView add_sdn(U8StringView sdn)
    {
        LDDValueView value = doc_->make_mutable()->parse_raw_value(sdn.begin(), sdn.end());
        array_.push_back(value.value_ptr_);
        return value;
    }

    void remove(size_t idx) {
        array_.remove(idx, 1);
    }

    void for_each(std::function<void(LDDValueView)> fn) const;


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
        LDDValueView vv = *this;
        return vv.clone(compactify);
    }

private:
    void do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;
};


static inline std::ostream& operator<<(std::ostream& out, const LDDArrayView& array)
{
    LDDumpFormatState format = LDDumpFormatState().simple();
    array.dump(out, format);
    return out;
}


template <>
struct DataTypeTraits<LDArray> {
    static constexpr bool isDataType = true;
    using LDStorageType = NullType;
    using LDViewType = LDDArrayView;

    static void create_signature(SBuf& buf) {
        buf << "LDDArrayView";
    }
};


template <typename Selector>
struct LDStorageAllocator<LDArray, Selector> {
    template <typename Arena>
    static auto allocate_and_construct(Arena* arena)
    {
        return LDDArrayView::Array::create_tagged_ptr(ld_tag_size<LDArray>(), arena, 4);
    }


    template <typename Arena>
    static auto allocate_and_construct(Arena* arena, size_t capacity)
    {
        return LDDArrayView::Array::create_tagged_ptr(ld_tag_size<LDArray>(), arena, capacity);
    }
};

}}
