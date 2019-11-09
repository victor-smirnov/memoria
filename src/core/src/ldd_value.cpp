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

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {

std::ostream& LDDValue::dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    if (is_null()) {
        out << "null";
    }
    else if (is_string())
    {
        U8StringView str = as_string().view();
        U8StringView str_escaped = SDNStringEscaper::current().escape_quotes(str);

        out << "'" << str_escaped << "'";

        SDNStringEscaper::current().reset();
    }
    else if (is_integer()) {
        out << as_integer();
    }
    else if (is_double()) {
        out << as_double();
    }
    else if (is_map()) {
        as_map().dump(out, state, dump_state);
    }
    else if (is_array()) {
        as_array().dump(out, state, dump_state);
    }
    else if (is_type_decl()) {
        as_type_decl().dump(out, state, dump_state);
    }
    else if (is_typed_value()) {
        as_typed_value().dump(out, state, dump_state);
    }
    else {
        out << "<unknown type>";
    }

    return out;
}


bool LDDValue::is_simple_layout() const noexcept
{
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


SDN2PtrHolder LDDValue::deep_copy_to(LDDocument* tgt, SDN2ArenaAddressMapping& mapping) const
{
    if (is_null()) {
        return LDDValue(tgt, 0).value_ptr_;
    }
    else if (is_string()) {
        return as_string().deep_copy_to(tgt, mapping);
    }
    else if (is_integer())
    {
        int64_t value = as_integer();
        return tgt->new_integer(value).value_ptr_;
    }
    else if (is_double())
    {
        double value = as_double();
        return tgt->new_double(value).value_ptr_;
    }
    else if (is_map()) {
        return as_map().deep_copy_to(tgt, mapping);
    }
    else if (is_array()) {
        return as_array().deep_copy_to(tgt, mapping);
    }
    else if (is_type_decl()) {
        return as_type_decl().deep_copy_to(tgt, mapping);
    }
    else if (is_typed_value()) {
        return as_typed_value().deep_copy_to(tgt, mapping);
    }

    MMA1_THROW(RuntimeException()) << WhatCInfo("Unknown linked data document value type");
}


SDN2Ptr<U8LinkedString> LDString::deep_copy_to(LDDocument* tgt, SDN2ArenaAddressMapping& mapping) const
{
    return tgt->new_string(string_.get(&doc_->arena_)->view()).string_;
}


LDDocument LDDValue::clone(bool compactify) const
{
    LDDocument tgt;
    SDN2ArenaAddressMapping mapping;

    LDDValue tgt_value(&tgt, deep_copy_to(&tgt, mapping));
    tgt.set_value(tgt_value);

    if (compactify) {
        tgt.compactify();
    }

    return tgt;
}

LDDocument LDString::clone(bool compactify) const {
    LDDValue vv = *this;
    return vv.clone(compactify);
}

}}
