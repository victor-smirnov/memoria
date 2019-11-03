
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
#include <memoria/v1/core/linked/document/ld_typed_value.hpp>


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

inline LDTypeDeclaration LDDValue::as_type_decl() const noexcept {
    return LDTypeDeclaration(doc_, value_ptr_);
}

inline LDDTypedValue LDDValue::as_typed_value() const noexcept {
    return LDDTypedValue(doc_, value_ptr_);
}

inline LDDMap LDDArray::add_map()
{
    ValueMap value = ValueMap::create(&doc_->arena_, sizeof(LDDValueTag));
    set_tag(value.ptr(), LDDValueTraits<LDDMap>::ValueTag);
    array_.push_back(value.ptr());
    return LDDMap(doc_, value);
}

inline bool LDDValue::is_simple_layout() const noexcept {
    if (is_string() || is_integer() || is_null()) {
        return true;
    }

    if (is_map()) {
        return as_map().is_simple_layout();
    }

    if (is_array()) {
        return as_array().is_simple_layout();
    }

    if (is_type_decl()) {
        return as_type_decl().is_simple_layout();
    }

    if (is_typed_value()) {
        return as_typed_value().is_simple_layout();
    }

    return false;
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

inline LDTypeDeclaration LDDocument::new_type_declaration(U8StringView name)
{
    auto td_ptr = allocate_tagged<sdn2_::TypeDeclState>(sizeof(LDDValueTag), &arena_, sdn2_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = intern(name);
    td_ptr.get(&arena_)->name = ss_ptr;

    ensure_type_decls_capacity(1).push_back(td_ptr);

    return LDTypeDeclaration(this, td_ptr);
}

inline LDTypeDeclaration LDDocument::new_type_declaration(LDIdentifier name)
{
    LDTypeDeclaration td = new_detached_type_declaration(name);
    ensure_type_decls_capacity(1).push_back(td.state_);
    return td;
}

inline LDTypeDeclaration LDDocument::new_detached_type_declaration(LDIdentifier name)
{
    auto td_ptr = allocate_tagged<sdn2_::TypeDeclState>(sizeof(LDDValueTag), &arena_, sdn2_::TypeDeclState{0, 0, 0});
    set_tag(td_ptr.get(), LDDValueTraits<LDTypeDeclaration>::ValueTag);

    auto ss_ptr = name.string_;
    td_ptr.get(&arena_)->name = ss_ptr;
    return LDTypeDeclaration(this, td_ptr);
}

inline LDDTypedValue LDDocument::new_typed_value(LDTypeDeclaration typedecl, LDDValue ctr_value)
{
    SDN2Ptr<sdn2_::TypedValueState> ss = allocate_tagged<sdn2_::TypedValueState>(
        sizeof(LDDValueTag), &arena_, sdn2_::TypedValueState{typedecl.state_, ctr_value.value_ptr_}
    );

    set_tag(ss.get(), LDDValueTraits<LDDTypedValue>::ValueTag);

    return LDDTypedValue{this, ss};
}

inline void LDDocument::for_each_type(std::function<void (LDTypeDeclaration)> fn) const
{
    auto* sst = state();
    if (sst->type_directory)
    {
        TypeDeclsVector decls = TypeDeclsVector::get(&arena_, sst->type_directory);
        decls.for_each([&](auto decl_ptr){
            LDTypeDeclaration td{const_cast<LDDocument*>(this), decl_ptr};
            fn(td);
        });
    }
}

inline std::ostream& LDDocument::dump(std::ostream& out, LDDumpState& state) const {
    value().dump(out, state);
    return out;
}


inline std::ostream& LDDValue::dump(std::ostream& out, LDDumpState& state) const
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
        as_map().dump(out, state);
    }
    else if (is_array()) {
        as_array().dump(out, state);
    }
    else if (is_type_decl()) {
        as_type_decl().dump(out, state);
    }
    else if (is_typed_value()) {
        as_typed_value().dump(out, state);
    }
    else {
        out << "<unknown type>";
    }

    return out;
}

inline LDString::operator LDDValue() const {
    return LDDValue{doc_, string_.get()};
}

inline LDDArray::operator LDDValue() const {
    return LDDValue{doc_, array_.ptr()};
}

}}
