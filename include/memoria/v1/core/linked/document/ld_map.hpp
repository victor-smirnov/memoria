
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
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_value.hpp>

namespace memoria {
namespace v1 {



class LDDMap {
    using ValueMap = sdn2_::ValueMap;
    using Array = sdn2_::Array;

    friend class LDTypeDeclaration;

    using PtrHolder = typename SDN2ArenaBase::PtrHolderT;
    const LDDocumentView* doc_;
    ValueMap map_;
public:
    LDDMap(): doc_(), map_() {}

    LDDMap(const LDDocumentView* doc, SDN2Ptr<ValueMap::State> map):
        doc_(doc), map_(doc_->arena_, map)
    {}

    LDDMap(const LDDocumentView* doc, ValueMap map):
        doc_(doc), map_(map)
    {}

    operator LDDValue() const {
        return as_value();
    }

    LDDValue as_value() const {
        return LDDValue{doc_, map_.ptr()};
    }

    Optional<LDDValue> get(U8StringView name) const
    {
        Optional<SDN2PtrHolder> ptr = map_.get(name);
        if (ptr) {
            return LDDValue(doc_, ptr.get());
        }
        else {
            return Optional<LDDValue>{};
        }
    }

    void set(U8StringView name, U8StringView value)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        SDN2Ptr<U8LinkedString> name_str  = mutable_doc->intern(name);
        SDN2Ptr<U8LinkedString> value_str = mutable_doc->intern(value);

        set_tag(value_str.get(), LDDValueTraits<LDString>::ValueTag);

        map_.put(name_str, value_str);
    }

    void set(U8StringView name, int64_t value)
    {
        SDN2Ptr<U8LinkedString> name_str  = doc_->make_mutable()->intern(name);

        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(LDDValueTag), doc_->arena_->make_mutable(), value);

        set_tag(value_ptr.get(), LDDValueTraits<int64_t>::ValueTag);

        map_.put(name_str, value_ptr);
    }

    void set(U8StringView name, double value)
    {
        SDN2Ptr<U8LinkedString> name_str  = doc_->make_mutable()->intern(name);

        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(LDDValueTag), doc_->arena_->make_mutable(), value);

        set_tag(value_ptr.get(), LDDValueTraits<double>::ValueTag);

        map_.put(name_str, value_ptr);
    }

    void set(LDString name, LDDValue value)
    {
        SDN2Ptr<U8LinkedString> name_str  = doc_->make_mutable()->intern(name.view());
        map_.put(name_str, value.value_ptr_);
    }

    LDDMap add_map(U8StringView name)
    {
        SDN2Ptr<U8LinkedString> name_str = doc_->make_mutable()->intern(name);
        ValueMap value = ValueMap::create(doc_->arena_->make_mutable(), sizeof(LDDValueTag));

        set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);

        map_.put(name_str, value.ptr());
        return LDDMap(doc_, value);
    }

    LDDArray add_array(U8StringView name)
    {
        SDN2Ptr<U8LinkedString> name_str = doc_->make_mutable()->intern(name);
        Array value =  Array::create_tagged(sizeof(LDDValueTag), doc_->arena_->make_mutable(), 4);
        set_tag(value.ptr(), LDDValueTraits<LDDArray>::ValueTag);

        map_.put(name_str, value.ptr());
        return LDDArray(doc_, value);
    }

    LDDValue add_sdn(U8StringView name, U8StringView sdn)
    {
        SDN2Ptr<U8LinkedString> name_str = doc_->make_mutable()->intern(name);
        LDDValue value = doc_->make_mutable()->parse_raw_value(sdn.begin(), sdn.end());

        map_.put(name_str, value.value_ptr_);

        return value;
    }

    void remove(U8StringView name)
    {
        map_.remove(name);
    }

    size_t size() const {
        return map_.size();
    }

    void for_each(std::function<void(U8StringView, LDDValue)> fn) const
    {
        map_.for_each([&](const auto& key, const auto& value){
            U8StringView kk = key.get(doc_->arena_)->view();
            fn(kk, LDDValue{doc_, value});
        });
    }

    std::ostream& dump(std::ostream& out) const
    {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        dump(out, state, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const
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


    bool is_simple_layout() const noexcept
    {
        if (size() > 2) {
            return false;
        }

        bool simple = true;

        for_each([&](auto key, auto vv){
            simple = simple && vv.is_simple_layout();
        });

        return simple;
    }

private:

    void do_dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const;

    void set_tag(SDN2PtrHolder ptr, LDDValueTag tag)
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        *tag_ptr.get_mutable(doc_->arena_) = tag;
    }

    LDDValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        return *tag_ptr.get(doc_->arena_);
    }
};


}}
