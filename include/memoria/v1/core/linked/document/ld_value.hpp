
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
    LDDocument* doc_;
    SDN2PtrHolder value_ptr_;
    LDDValueTag type_tag_;

    friend class LDDocument;
    friend class LDDMap;
    friend class LDDArray;

public:
    LDDValue() noexcept : doc_(), value_ptr_(), type_tag_() {}

    LDDValue(LDDocument* doc, SDN2PtrHolder value_ptr) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(value_ptr ? get_tag(value_ptr) : 0)
    {}

    LDDMap as_map() const noexcept;
    LDDArray as_array() const noexcept;

    LDString as_string() const noexcept {
        return LDString(doc_, value_ptr_);
    }

    int64_t as_integer() const noexcept {
        return *SDN2Ptr<int64_t>(value_ptr_).get(&doc_->arena_);
    }

    double as_double() const noexcept {
        return *SDN2Ptr<double>(value_ptr_).get(&doc_->arena_);
    }

    bool is_null() const noexcept {
        return value_ptr_ == 0;
    }

    bool is_integer() const noexcept {
        return type_tag_ == LDDValueTraits<int64_t>::ValueTag;
    }

    bool is_double() const noexcept {
        return type_tag_ == LDDValueTraits<double>::ValueTag;
    }

    bool is_string() const noexcept {
        return type_tag_ == LDDValueTraits<LDString>::ValueTag;
    }

    bool is_array() const noexcept {
        return type_tag_ == LDDValueTraits<LDDArray>::ValueTag;
    }

    bool is_map() const noexcept {
        return type_tag_ == LDDValueTraits<LDDMap>::ValueTag;
    }

    LDDValueTag tag() const {
        return type_tag_;
    }

    void dump(std::ostream& out, size_t indent = 0) const;

private:

    LDDValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<LDDValueTag> tag_ptr(ptr - sizeof(LDDValueTag));
        return *tag_ptr.get(&doc_->arena_);
    }
};


}}
