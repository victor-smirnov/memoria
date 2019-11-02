
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

#include <memoria/v1/core/linked/document/ld_value.hpp>
#include <memoria/v1/core/linked/document/ld_string.hpp>
#include <memoria/v1/core/linked/document/ld_identifier.hpp>
#include <memoria/v1/core/linked/document/ld_map.hpp>
#include <memoria/v1/core/linked/document/ld_array.hpp>
#include <memoria/v1/core/linked/document/ld_type_decl.hpp>


#include <memoria/v1/core/tools/optional.hpp>

#include <boost/variant.hpp>

namespace memoria {
namespace v1 {








inline LDDArray LDDValue::as_array() const noexcept {
    return LDDArray(doc_, value_ptr_);
}


inline LDDMap LDDValue::as_map() const noexcept {
    return LDDMap(doc_, value_ptr_);
}


inline LDDMap LDDArray::add_map()
{
    ValueMap value = ValueMap::create(&doc_->arena_, sizeof(LDDValueTag));
    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);
    array_.push_back(value.ptr());
    return LDDMap(doc_, value);
}


inline SDN2Ptr<U8LinkedString> LDDocument::intern(U8StringView string)
{
    StringSet set;

    auto* sst = state();

    if (!sst->strings) {
        set = StringSet::create(&arena_);
        state()->strings = set.ptr();
    }
    else {
        set = StringSet{&arena_, sst->strings};
    }

    Optional<SDN2Ptr<U8LinkedString>> str = set.get(string);
    if (str)
    {
        return str.get();
    }
    else {
        SDN2Ptr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), &arena_, string);
        set.put(ss);
        return ss;
    }
}

inline LDString LDDocument::new_string(U8StringView view)
{
    SDN2Ptr<U8LinkedString> ptr = intern(view);
    set_tag(ptr.get(), LDDValueTraits<LDString>::ValueTag);
    return LDString{this, ptr};
}

inline LDIdentifier LDDocument::new_identifier(U8StringView view)
{
    SDN2Ptr<U8LinkedString> ptr = intern(view);
    set_tag(ptr.get(), LDDValueTraits<LDIdentifier>::ValueTag);
    return LDIdentifier{this, ptr};
}

inline LDDValue LDDocument::new_integer(int64_t value)
{
    SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(LDDValueTag), &arena_, value);
    set_tag(value_ptr.get(), LDDValueTraits<int64_t>::ValueTag);
    return LDDValue{this, value_ptr};
}

inline LDDValue LDDocument::new_double(double value)
{
    SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(LDDValueTag), &arena_, value);
    set_tag(value_ptr.get(), LDDValueTraits<double>::ValueTag);
    return LDDValue{this, value_ptr};
}

inline LDDArray LDDocument::new_array(Span<LDDValue> span)
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), &arena_, span.length());
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    for (const LDDValue& vv: span) {
        array.push_back(vv.value_ptr_);
    }

    return LDDArray(this, array.ptr());
}

inline LDDArray LDDocument::new_array()
{
    Array array = Array::create_tagged(sizeof(LDDValueTag), &arena_, 4);
    set_tag(array.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    return LDDArray(this, array.ptr());
}


inline LDDMap LDDocument::new_map()
{
    ValueMap value = ValueMap::create(&arena_, sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);

    return LDDMap(this, value);
}

inline void LDDocument::set_value(LDDValue value)
{
    state()->value = value.value_ptr_;
}


inline LDDValue LDDocument::value() const {
    return LDDValue{const_cast<LDDocument*>(this), state()->value};
}

inline void LDDocument::set(U8StringView string)
{
    SDN2Ptr<U8LinkedString> ss = allocate_tagged<U8LinkedString>(sizeof(LDDValueTag), &arena_, string);
    set_tag(ss.get(), LDDValueTraits<LDString>::ValueTag);
    state()->value = ss.get();
}

inline void LDDocument::set(int64_t value)
{
    SDN2Ptr<int64_t> ss = allocate_tagged<int64_t>(sizeof(LDDValueTag), &arena_, value);
    set_tag(ss.get(), LDDValueTraits<int64_t>::ValueTag);
    state()->value = ss;
}

inline void LDDocument::set(double value)
{
    SDN2Ptr<double> ss = allocate_tagged<double>(sizeof(LDDValueTag), &arena_, value);
    set_tag(ss.get(), LDDValueTraits<double>::ValueTag);
    state()->value = ss;
}

inline LDDMap LDDocument::set_map()
{
    ValueMap value = ValueMap::create(&arena_, sizeof(LDDValueTag));

    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);
    state()->value = value.ptr();

    return LDDMap(this, value);
}

inline LDDArray LDDocument::set_array()
{
    Array value = Array::create_tagged(sizeof(LDDValueTag), &arena_, 4);
    set_tag(value.ptr(), LDDValueTraits<LDDArray>::ValueTag);

    state()->value = value.ptr();

    return LDDArray(this, value);
}

inline std::ostream& LDDocument::dump(std::ostream& out) const {
    value().dump(out);
    return out;
}


inline void LDDValue::dump(std::ostream& out, size_t indent) const
{
    if (is_null()) {
        out << "null";
    }
    else if (is_string()) {
        out << "'" << as_string().view() << "'";
    }
    else if (is_integer()) {
        out << as_integer();
    }
    else if (is_double()) {
        out << as_double();
    }
    else if (is_map()) {
        as_map().dump(out, indent);
    }
    else if (is_array()) {
        as_array().dump(out, indent);
    }
    else {
        out << "<unknown type>";
    }
}

inline LDString::operator LDDValue() const {
    return LDDValue{doc_, string_.get()};
}

inline LDDArray::operator LDDValue() const {
    return LDDValue{doc_, array_.ptr()};
}

}}
