
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
    using Array = ld_::Array;
    using ValueMap = ld_::ValueMap;

    friend class LDTypeDeclaration;
    friend class LDDMap;
    friend class LDDocument;

    using PtrHolder = typename ld_::LDArenaView::PtrHolderT;
    const LDDocumentView* doc_;
    Array array_;

public:
    LDDArray(): doc_(), array_() {}

    LDDArray(const LDDocumentView* doc, Array array):
        doc_(doc), array_(array)
    {}

    LDDArray(const LDDocumentView* doc, PtrHolder ptr):
        doc_(doc), array_(Array::get(&doc_->arena_, ptr))
    {}

    operator LDDValue() const;

    LDDValue get(size_t idx) const;



    void set(size_t idx, U8StringView value)
    {
        LDString str = doc_->make_mutable()->new_string(value);
        array_.access(idx) = str.string_.get();
    }

    void set(size_t idx, int64_t value)
    {
        LDDValue vv = doc_->make_mutable()->new_integer(value);
        array_.access(idx) = vv.value_ptr_;
    }

    void set(size_t idx, double value)
    {
        LDDValue vv = doc_->make_mutable()->new_double(value);
        array_.access(idx) = vv.value_ptr_;
    }

    void add(U8StringView value)
    {
        LDString str = doc_->make_mutable()->new_string(value);
        array_.push_back(str.string_.get());
    }

    void add(int64_t value)
    {
        LDDValue vv = doc_->make_mutable()->new_integer(value);
        array_.push_back(vv.value_ptr_);
    }

    void add(double value)
    {
        LDDValue vv = doc_->make_mutable()->new_double(value);
        array_.push_back(vv.value_ptr_);
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

}}
