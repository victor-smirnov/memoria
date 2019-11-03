
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



class LDDArray {
    using Array = sdn2_::Array;
    using ValueMap = sdn2_::ValueMap;

    friend class LDTypeDeclaration;

    using PtrHolder = typename SDN2ArenaBase::PtrHolderT;
    LDDocument* doc_;
    Array array_;

public:
    LDDArray(): doc_(), array_() {}

    LDDArray(LDDocument* doc, Array array):
        doc_(doc), array_(array)
    {}

    LDDArray(LDDocument* doc, PtrHolder ptr):
        doc_(doc), array_(Array::get(&doc_->arena_, ptr))
    {}

    operator LDDValue() const;

    LDDValue get(size_t idx) const
    {
        SDN2PtrHolder ptr = array_.access(idx);
        return LDDValue{doc_, ptr};
    }


    void set(size_t idx, U8StringView value)
    {
        SDN2Ptr<U8LinkedString> value_str = doc_->intern(value);
        set_tag(value_str.get(), LDDValueTraits<LDString>::ValueTag);
        array_.access(idx) = value_str;
    }

    void set(size_t idx, int64_t value)
    {
        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(LDDValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), LDDValueTraits<int64_t>::ValueTag);
        array_.access(idx) = value_ptr;
    }

    void set(size_t idx, double value)
    {
        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(LDDValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), LDDValueTraits<double>::ValueTag);
        array_.access(idx) = value_ptr;
    }

    void add(U8StringView value)
    {
        SDN2Ptr<U8LinkedString> value_str = doc_->intern(value);
        set_tag(value_str.get(), LDDValueTraits<LDString>::ValueTag);
        array_.push_back(value_str);
    }

    void add(int64_t value)
    {
        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(LDDValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), LDDValueTraits<int64_t>::ValueTag);
        array_.push_back(value_ptr);
    }

    void add(double value)
    {
        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(LDDValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), LDDValueTraits<double>::ValueTag);
        array_.push_back(value_ptr);
    }

    LDDMap add_map();

    LDDArray add_array()
    {
        Array value = Array::create_tagged(sizeof(LDDValueTag), &doc_->arena_, 4);
        set_tag(value.ptr(), LDDValueTraits<LDDArray>::ValueTag);
        array_.push_back(value.ptr());
        return LDDArray(doc_, value);
    }

    void remove(size_t idx)
    {
        array_.remove(idx, 1);
    }

    void for_each(std::function<void(LDDValue)> fn) const
    {
        array_.for_each([&](const auto& value){
            fn(LDDValue{doc_, value});
        });
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpState state;
        dump(out, state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpState& state) const
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_dump(out, state);
        }
        else {
            LDDumpState state = LDDumpState::simple();
            do_dump(out, state);
        }

        return out;
    }

    size_t size() const noexcept {
        return array_.size();
    }

    bool is_simple_layout() const noexcept
    {
        if (size() > 3) {
            return false;
        }

        bool simple = true;

        for_each([&](auto vv){
            simple = simple && vv.is_simple_layout();
        });

        return simple;
    }

private:

    void do_dump(std::ostream& out, LDDumpState& state) const;


    void set_tag(SDN2PtrHolder ptr, LDDValueTag tag) noexcept
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        *tag_ptr.get(&doc_->arena_) = tag;
    }

    LDDValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        return *tag_ptr.get(&doc_->arena_);
    }
};

}}
