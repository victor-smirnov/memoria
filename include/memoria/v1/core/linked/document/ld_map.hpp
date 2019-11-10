
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
    using ValueMap = ld_::ValueMap;
    using Array = ld_::Array;

    friend class LDTypeDeclaration;
    friend class LDDArray;


    using PtrHolder = typename ld_::LDArenaView::PtrHolderT;
    const LDDocumentView* doc_;
    ValueMap map_;
public:
    LDDMap(): doc_(), map_() {}

    LDDMap(const LDDocumentView* doc, ld_::LDPtr<ValueMap::State> map):
        doc_(doc), map_(&doc_->arena_, map)
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
        Optional<ld_::LDGenericPtr<ld_::GenericValue>> ptr = map_.get(name);
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

        LDString name_str  = mutable_doc->new_string(name);
        LDString value_str = mutable_doc->new_string(value);

        map_.put(name_str.string_, value_str.string_);
    }

    void set(U8StringView name, int64_t value)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDString name_str  = mutable_doc->new_string(name);
        LDDValue value_vv  = mutable_doc->new_integer(value);

        map_.put(name_str.string_, value_vv.value_ptr_);
    }

    void set(U8StringView name, double value)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDString name_str  = mutable_doc->new_string(name);
        LDDValue value_vv  = mutable_doc->new_double(value);

        map_.put(name_str.string_, value_vv.value_ptr_);

    }

    void set(LDString name, LDDValue value)
    {
        map_.put(name.string_, value.value_ptr_);
    }

    LDDMap add_map(U8StringView name)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDString name_str  = mutable_doc->new_string(name);
        LDDMap map = mutable_doc->new_map();

        map_.put(name_str.string_, map.map_.ptr());
        return map;
    }

    LDDArray add_array(U8StringView name)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDString name_str  = mutable_doc->new_string(name);
        LDDArray array = mutable_doc->new_array();

        map_.put(name_str.string_, array.array_.ptr());
        return array;
    }

    LDDValue add_sdn(U8StringView name, U8StringView sdn)
    {
        LDDocumentView* mutable_doc = doc_->make_mutable();

        LDString name_str   = mutable_doc->new_string(name);
        LDDValue value      = mutable_doc->parse_raw_value(sdn.begin(), sdn.end());

        map_.put(name_str.string_, value.value_ptr_);

        return value;
    }

//    LDDValue add_document(U8StringView name, const LDDocument& source)
//    {
//        LDDocumentView* mutable_doc = doc_->make_mutable();

//        ld_::LDArenaAddressMapping mapping;
//        ld_::LDDPtrHolder ptr = source.value().deep_copy_to(doc_->make_mutable(), mapping);

//        map_.put(name_str, ptr);

//        return LDDValue{doc_, ptr};
//    }

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
            U8StringView kk = key.get(&doc_->arena_)->view();
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

    ld_::LDPtr<ValueMap::State> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    LDDocument clone(bool compactify = true) const {
        LDDValue vv = *this;
        return vv.clone(compactify);
    }

private:

    void do_dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const;
};


}}
